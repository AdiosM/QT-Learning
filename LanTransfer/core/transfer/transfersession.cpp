#include "transfersession.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QUuid>

#include "protocol/packet.h"
#include "protocol/packetcodec.h"
#include "security/pairingmanager.h"

namespace lantransfer {

namespace {
// 各阶段超时（设计文档 §11）
constexpr int HELLO_TIMEOUT_MS = 15000;        // 握手
constexpr int USER_ACCEPT_TIMEOUT_MS = 120000; // 人工确认
constexpr int FILE_ACCEPT_TIMEOUT_MS = 60000;  // 等 FILE_INFO/FILE_ACCEPT
constexpr int DATA_IDLE_TIMEOUT_MS = 30000;    // 数据空闲
constexpr int PROGRESS_INTERVAL_MS = 250;      // 进度刷新（§19 200-500ms）
} // namespace

TransferSession::TransferSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
{
    Q_ASSERT(socket);
    m_socket->setParent(this);
    m_chunkBuf.resize(DEFAULT_CHUNK_SIZE);

    m_idleTimer.setSingleShot(true);
    connect(&m_idleTimer, &QTimer::timeout, this, &TransferSession::onTimeout);
    connect(m_socket, &QTcpSocket::readyRead, this, &TransferSession::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &TransferSession::onSocketBytesWritten);
    connect(m_socket, &QTcpSocket::disconnected, this, &TransferSession::onSocketDisconnected);
    m_progressClock.start();
}

TransferSession::~TransferSession()
{
    // 失败/未完成时清理临时文件（成功路径已在 commit 后置空 tempPath）
    if (m_sink.isOpen() || !m_sink.tempPath().isEmpty())
        m_sink.abort();
}

void TransferSession::setLocalInfo(const QString& deviceId, const QString& deviceName,
                                   const QString& deviceType, quint16 tcpPort,
                                   const QString& appVersion)
{
    m_localDeviceId = deviceId;
    m_localDeviceName = deviceName;
    m_localDeviceType = deviceType;
    m_localTcpPort = tcpPort;
    m_localAppVersion = appVersion;
}

QString TransferSession::peerIp() const
{
    return m_socket ? m_socket->peerAddress().toString() : QString();
}

quint64 TransferSession::transferredBytes() const
{
    return m_role == TransferRole::Sender ? m_sentBytes : m_receivedBytes;
}

// ---------------------------------------------------------------------------
// 角色初始化
// ---------------------------------------------------------------------------
void TransferSession::startAsSender(const QString& peerDeviceId,
                                    std::shared_ptr<FileSource> source,
                                    const QString& fileName, qint64 fileSize)
{
    m_role = TransferRole::Sender;
    m_peerDeviceId = peerDeviceId;
    m_source = std::move(source);
    m_fileName = fileName;
    m_fileSize = quint64(fileSize);
    m_transferId = QUuid::createUuid();
    m_transferIdEstablished = true;

    setState(TransferState::Handshaking);
    sendHello();
    resetIdleTimer(HELLO_TIMEOUT_MS);
    emit logMessage(QStringLiteral("已连接 %1，正在握手…").arg(peerIp()));
}

void TransferSession::startAsReceiver(const QString& receiveDir)
{
    m_role = TransferRole::Receiver;
    m_receiveDir = receiveDir;
    setState(TransferState::Handshaking);
    resetIdleTimer(HELLO_TIMEOUT_MS);
    emit logMessage(QStringLiteral("收到来自 %1 的连接，等待握手…").arg(peerIp()));
}

// ---------------------------------------------------------------------------
// 用户操作
// ---------------------------------------------------------------------------
void TransferSession::userAcceptPair(bool accepted)
{
    if (m_role != TransferRole::Receiver || m_state != TransferState::WaitingUserAccept)
        return;

    const PairResponsePayload resp{accepted, accepted ? m_pin : QString()};
    sendControl(MessageType::PairResponse, resp.toJson());
    m_idleTimer.stop();

    if (!accepted) {
        fail(ErrorCode::UserRejected, QStringLiteral("已拒绝对方的传输请求"), false);
        return;
    }
    setState(TransferState::WaitingFileAccept);
    resetIdleTimer(FILE_ACCEPT_TIMEOUT_MS);
    emit logMessage(QStringLiteral("已接受 %1 的传输请求").arg(m_peerDeviceName));
}

void TransferSession::userConfirmPin(const QString& pin)
{
    if (m_role != TransferRole::Sender || m_expectedPin.isEmpty()
        || m_state != TransferState::WaitingUserAccept)
        return;

    if (!PairingManager::verifyPin(m_expectedPin, pin)) {
        sendControl(MessageType::Cancel, CancelPayload{QStringLiteral("pin_mismatch")}.toJson());
        fail(ErrorCode::PinMismatch, QStringLiteral("配对 PIN 不匹配，已取消传输"), false);
        return;
    }
    m_expectedPin.clear();
    sendFileInfo();
}

void TransferSession::userResolveConflict(ConflictResolution resolution)
{
    if (m_role != TransferRole::Receiver || m_pendingFileInfo.fileId.isEmpty())
        return;

    const FileInfoPayload info = m_pendingFileInfo;
    m_pendingFileInfo = FileInfoPayload{};

    if (resolution == ConflictResolution::Cancel) {
        sendControl(MessageType::Cancel, CancelPayload{QStringLiteral("user")}.toJson());
        fail(ErrorCode::Cancelled, QStringLiteral("已取消传输"), false);
        return;
    }
    m_conflictPolicy = (resolution == ConflictResolution::Overwrite)
        ? ConflictPolicy::Overwrite : ConflictPolicy::AutoRename;
    acceptFileInfo(info);
}

void TransferSession::userCancel()
{
    if (isTransferFinished(m_state) || m_closing)
        return;
    sendControl(MessageType::Cancel, CancelPayload{QStringLiteral("user")}.toJson());
    fail(ErrorCode::Cancelled, QStringLiteral("已取消传输"), false);
}

void TransferSession::rejectWithError(ErrorCode code, const QString& message)
{
    if (m_closing)
        return;
    sendErrorToPeer(code, message);
    fail(code, message, false);
}

// ---------------------------------------------------------------------------
// 帧分发
// ---------------------------------------------------------------------------
void TransferSession::onSocketReadyRead()
{
    if (m_closing)
        return;
    m_parser.append(m_socket->readAll());

    PacketParser::Packet packet;
    while (m_parser.takePacket(&packet)) {
        if (m_closing)
            return;
        // 帧头版本校验
        if (packet.header.version != PROTOCOL_VERSION) {
            fail(ErrorCode::ProtocolError, QStringLiteral("协议版本不匹配"), true);
            return;
        }
        // TransferId 一致性：首帧确立，后续必须一致
        if (!m_transferIdEstablished) {
            m_transferId = packet.header.transferId;
            m_transferIdEstablished = true;
        } else if (packet.header.transferId != m_transferId) {
            fail(ErrorCode::ProtocolError, QStringLiteral("TransferId 不一致"), true);
            return;
        }
        dispatchPacket(packet);
    }

    if (m_parser.lastError() != PacketParser::Error::None) {
        fail(ErrorCode::ProtocolError, QStringLiteral("协议帧解析错误"), true);
    }
}

void TransferSession::dispatchPacket(const PacketParser::Packet& packet)
{
    const MessageType type = static_cast<MessageType>(packet.header.type);

    // FILE_DATA 为原始字节，不经 JSON
    if (type == MessageType::FileData) {
        handleFileData(packet.payload);
        return;
    }

    const auto json = PacketCodec::decodeControlJson(packet.payload);
    if (!json.has_value()) {
        fail(ErrorCode::ProtocolError, QStringLiteral("控制消息 JSON 解析失败"), true);
        return;
    }

    switch (type) {
    case MessageType::Hello:        handleHello(*json);        break;
    case MessageType::PairRequest:  handlePairRequest(*json);  break;
    case MessageType::PairResponse: handlePairResponse(*json); break;
    case MessageType::FileInfo:     handleFileInfo(*json);     break;
    case MessageType::FileAccept:   handleFileAccept(*json);   break;
    case MessageType::FileEnd:      handleFileEnd(*json);      break;
    case MessageType::FileVerify:   handleFileVerify(*json);   break;
    case MessageType::Cancel:       handleCancel(*json);       break;
    case MessageType::Error:        handleError(*json);        break;
    case MessageType::SessionClose: closeGracefully();         break;
    case MessageType::FileData:     break; // 已在上面处理
    }
}

// ---------------------------------------------------------------------------
// 发送动作
// ---------------------------------------------------------------------------
void TransferSession::sendHello()
{
    const HelloPayload hello{
        m_localDeviceId, m_localDeviceName, m_localDeviceType,
        m_localTcpPort, m_localAppVersion, {QStringLiteral("file-transfer")},
    };
    sendControl(MessageType::Hello, hello.toJson());
}

void TransferSession::sendPairRequest()
{
    const PairRequestPayload req{m_localDeviceName, m_localAppVersion, m_fileName, m_fileSize};
    sendControl(MessageType::PairRequest, req.toJson());
    setState(TransferState::WaitingUserAccept);
    resetIdleTimer(USER_ACCEPT_TIMEOUT_MS);
}

void TransferSession::sendFileInfo()
{
    // 发送前二次校验实际大小（等待期间文件可能变化，设计文档 §7.2）
    const qint64 actualSize = m_source->size();
    if (actualSize < 0 || actualSize > qint64(MAX_FILE_SIZE)) {
        fail(ErrorCode::FileTooLarge, QStringLiteral("文件超过 500 MB 限制"), true);
        return;
    }
    m_fileSize = quint64(actualSize);

    QMimeDatabase mimeDb;
    const FileInfoPayload info{
        QUuid::createUuid().toString(QUuid::WithoutBraces),
        m_fileName,
        m_fileSize,
        mimeDb.mimeTypeForFile(m_fileName).name(),
        QString(), // 初始握手不携带；流式 SHA-256 随 FILE_END 发送
        QString(), // V1.0 单文件固定为空
    };
    sendControl(MessageType::FileInfo, info.toJson());
    setState(TransferState::WaitingFileAccept);
    resetIdleTimer(FILE_ACCEPT_TIMEOUT_MS);
}

void TransferSession::sendControl(MessageType type, const QJsonObject& payload)
{
    if (m_closing || m_socket->state() != QAbstractSocket::ConnectedState)
        return;
    const QByteArray frame = PacketCodec::encodeControl(type, m_transferId, payload);
    if (frame.isEmpty()) {
        fail(ErrorCode::InternalError, QStringLiteral("控制帧超出大小上限"), true);
        return;
    }
    m_socket->write(frame);
}

void TransferSession::sendErrorToPeer(ErrorCode code, const QString& message)
{
    const ErrorPayload err{qint32(code), message};
    sendControl(MessageType::Error, err.toJson());
}

// ---------------------------------------------------------------------------
// 发送方：数据泵 + 流控（设计文档 §9.2）
// ---------------------------------------------------------------------------
void TransferSession::pumpFileData()
{
    if (m_role != TransferRole::Sender || m_state != TransferState::Transferring || m_closing)
        return;

    while (!m_eof && m_socket->bytesToWrite() < SEND_HIGH_WATERMARK) {
        const qint64 n = m_source->read(m_chunkBuf.data(), DEFAULT_CHUNK_SIZE);
        if (n < 0) {
            fail(ErrorCode::NotFound, QStringLiteral("读取文件失败：%1").arg(m_source->errorString()), true);
            return;
        }
        if (n == 0) {
            m_eof = true;
            break;
        }
        QByteArray chunk(m_chunkBuf.constData(), n);
        m_senderHash.addData(chunk);
        m_sentBytes += quint64(n);

        const QByteArray frame = PacketCodec::encodeFileData(m_transferId, chunk);
        if (frame.isEmpty()) {
            fail(ErrorCode::InternalError, QStringLiteral("数据帧组装失败"), true);
            return;
        }
        m_socket->write(frame);
        emitProgress();
        resetIdleTimer(DATA_IDLE_TIMEOUT_MS);
    }

    if (m_eof && m_state == TransferState::Transferring) {
        const FileEndPayload end{m_senderHash.resultHex()};
        sendControl(MessageType::FileEnd, end.toJson());
        setState(TransferState::Verifying);
        resetIdleTimer(DATA_IDLE_TIMEOUT_MS);
        emit logMessage(QStringLiteral("文件已发送，等待对端校验…"));
    }
}

void TransferSession::onSocketBytesWritten(qint64 /*bytes*/)
{
    if (m_role != TransferRole::Sender || m_closing)
        return;
    // 低于低水位则恢复读文件（慢接收端背压）
    if (m_state == TransferState::Transferring && !m_eof
        && m_socket->bytesToWrite() < SEND_LOW_WATERMARK) {
        pumpFileData();
    }
}

// ---------------------------------------------------------------------------
// 接收动作
// ---------------------------------------------------------------------------
void TransferSession::acceptFileInfo(const FileInfoPayload& info)
{
    m_fileName = info.fileName;
    m_fileSize = info.fileSize;

    // 接收端二次校验 500MB 上限（设计文档 §7.3，即使发送端已校验）
    if (info.fileSize > MAX_FILE_SIZE) {
        sendErrorToPeer(ErrorCode::FileTooLarge, QStringLiteral("文件超过 500 MB 限制"));
        fail(ErrorCode::FileTooLarge, QStringLiteral("文件超过 500 MB 限制"), false);
        return;
    }
    // 文件名安全（设计文档 §12）
    const FileNameCheckResult check = validateFileName(info.fileName);
    if (!check.ok) {
        sendErrorToPeer(ErrorCode::InvalidName, check.error);
        fail(ErrorCode::InvalidName, check.error, false);
        return;
    }

    // 目标文件冲突：询问用户（覆盖/重命名/取消，设计文档 §11）
    if (QFileInfo::exists(QDir(m_receiveDir).filePath(info.fileName))) {
        m_pendingFileInfo = info; // 等待 userResolveConflict 决策后继续
        emit fileConflict(info.fileName);
        resetIdleTimer(USER_ACCEPT_TIMEOUT_MS);
        return;
    }
    m_conflictPolicy = ConflictPolicy::AutoRename;

    if (!m_sink.open(m_receiveDir, info.fileName, m_conflictPolicy)) {
        sendErrorToPeer(ErrorCode::DiskFull, m_sink.errorString());
        fail(ErrorCode::DiskFull, m_sink.errorString(), false);
        return;
    }
    m_receivedBytes = 0;
    m_receiverHash.reset();

    sendControl(MessageType::FileAccept, FileAcceptPayload{true}.toJson());
    setState(TransferState::Transferring);
    resetIdleTimer(DATA_IDLE_TIMEOUT_MS);
    emit logMessage(QStringLiteral("开始接收：%1（%2）").arg(info.fileName).arg(info.fileSize));
}

void TransferSession::handleFileData(const QByteArray& payload)
{
    if (m_role != TransferRole::Receiver || m_state != TransferState::Transferring)
        return;

    if (!m_sink.write(payload.constData(), payload.size())) {
        sendErrorToPeer(ErrorCode::DiskFull, m_sink.errorString());
        fail(ErrorCode::DiskFull, m_sink.errorString(), false);
        return;
    }
    // fileSize + 已接收不得溢出，且不得超过声明大小（设计文档 §7.3）
    if (m_receivedBytes + quint64(payload.size()) > m_fileSize) {
        sendErrorToPeer(ErrorCode::Incomplete, QStringLiteral("接收数据超出声明大小"));
        fail(ErrorCode::Incomplete, QStringLiteral("接收数据超出声明大小"), false);
        return;
    }
    m_receivedBytes += quint64(payload.size());
    m_receiverHash.addData(payload);
    emitProgress();
    resetIdleTimer(DATA_IDLE_TIMEOUT_MS);
}

// ---------------------------------------------------------------------------
// 消息处理
// ---------------------------------------------------------------------------
void TransferSession::handleHello(const QJsonObject& obj)
{
    if (m_state != TransferState::Handshaking && m_state != TransferState::Connecting)
        return;
    const HelloPayload hello = HelloPayload::fromJson(obj);
    if (hello.deviceId.isEmpty()) {
        fail(ErrorCode::ProtocolError, QStringLiteral("HELLO 缺少设备ID"), true);
        return;
    }
    m_peerDeviceId = hello.deviceId;
    m_peerDeviceName = hello.deviceName;
    m_peerType = hello.deviceType;
    m_peerHelloReceived = true;

    if (m_role == TransferRole::Sender) {
        // 发送方收到对端 HELLO 后发起配对请求
        sendPairRequest();
    } else {
        // 接收方回复 HELLO（双方互发，设计文档 §8.1）
        sendHello();
        emit logMessage(QStringLiteral("与 %1(%2) 握手完成").arg(m_peerDeviceName, m_peerDeviceId.left(8)));
    }
}

void TransferSession::handlePairRequest(const QJsonObject& obj)
{
    if (m_role != TransferRole::Receiver)
        return;
    // 必须先完成 HELLO 交换（设备详细身份以 TCP HELLO 确认，设计文档 §5.2）
    if (!m_peerHelloReceived || m_state != TransferState::Handshaking)
        return;
    const PairRequestPayload req = PairRequestPayload::fromJson(obj);
    m_fileName = req.fileName;
    m_fileSize = req.fileSize;

    // 接收端在配对阶段即校验 500MB 上限（设计文档 §7.3）
    if (req.fileSize > MAX_FILE_SIZE) {
        sendErrorToPeer(ErrorCode::FileTooLarge, QStringLiteral("文件超过 500 MB 限制"));
        fail(ErrorCode::FileTooLarge, QStringLiteral("文件超过 500 MB 限制"), false);
        return;
    }

    m_pin = PairingManager::generatePin();
    setState(TransferState::WaitingUserAccept);
    resetIdleTimer(USER_ACCEPT_TIMEOUT_MS);
    emit pairRequestReceived(m_peerDeviceName, req.fileName, req.fileSize, m_pin);
}

void TransferSession::handlePairResponse(const QJsonObject& obj)
{
    if (m_role != TransferRole::Sender || m_state != TransferState::WaitingUserAccept)
        return;
    m_idleTimer.stop();
    const PairResponsePayload resp = PairResponsePayload::fromJson(obj);

    if (!resp.accepted) {
        fail(ErrorCode::UserRejected, QStringLiteral("对方拒绝传输"), false);
        return;
    }
    if (!resp.pin.isEmpty()) {
        m_expectedPin = resp.pin;
        emit pinRequired(resp.pin);
        resetIdleTimer(USER_ACCEPT_TIMEOUT_MS);
        return;
    }
    sendFileInfo();
}

void TransferSession::handleFileInfo(const QJsonObject& obj)
{
    if (m_role != TransferRole::Receiver)
        return;
    const FileInfoPayload info = FileInfoPayload::fromJson(obj);
    if (info.fileName.isEmpty()) {
        sendErrorToPeer(ErrorCode::InvalidName, QStringLiteral("文件名非法"));
        fail(ErrorCode::InvalidName, QStringLiteral("文件名非法"), false);
        return;
    }
    acceptFileInfo(info);
}

void TransferSession::handleFileAccept(const QJsonObject& obj)
{
    if (m_role != TransferRole::Sender || m_state != TransferState::WaitingFileAccept)
        return;
    m_idleTimer.stop();
    const FileAcceptPayload accept = FileAcceptPayload::fromJson(obj);

    if (!accept.accepted) {
        fail(ErrorCode::UserRejected, QStringLiteral("对方拒绝接收"), false);
        return;
    }
    if (!m_source->open()) {
        sendErrorToPeer(ErrorCode::NotFound,
                        QStringLiteral("无法打开文件：%1").arg(m_source->errorString()));
        fail(ErrorCode::NotFound, QStringLiteral("无法打开文件：%1").arg(m_source->errorString()), false);
        return;
    }
    setState(TransferState::Transferring);
    resetIdleTimer(DATA_IDLE_TIMEOUT_MS);
    emit logMessage(QStringLiteral("开始发送：%1（%2）").arg(m_fileName).arg(m_fileSize));
    pumpFileData();
}

void TransferSession::handleFileEnd(const QJsonObject& obj)
{
    if (m_role != TransferRole::Receiver || m_state != TransferState::Transferring)
        return;
    m_idleTimer.stop();
    const FileEndPayload end = FileEndPayload::fromJson(obj);

    // 完整性检查：receivedBytes 必须等于 fileSize（设计文档 §7.3）
    if (m_receivedBytes != m_fileSize) {
        sendErrorToPeer(ErrorCode::Incomplete,
                        QStringLiteral("接收字节数(%1)与声明大小(%2)不一致")
                            .arg(m_receivedBytes).arg(m_fileSize));
        fail(ErrorCode::Incomplete,
             QStringLiteral("接收字节数(%1)与声明大小(%2)不一致")
                 .arg(m_receivedBytes).arg(m_fileSize),
             false);
        return;
    }

    // SHA-256 对比（设计文档 §8.3）：与发送端流式计算值比较
    const QString localHex = m_receiverHash.resultHex();
    const bool match = end.sha256.isEmpty() || end.sha256.compare(localHex, Qt::CaseInsensitive) == 0;

    if (match) {
        if (!m_sink.commit()) {
            sendErrorToPeer(ErrorCode::DiskFull, m_sink.errorString());
            fail(ErrorCode::DiskFull, m_sink.errorString(), false);
            return;
        }
        m_finalPath = m_sink.finalPath();
        setState(TransferState::Completed);
        sendControl(MessageType::FileVerify, FileVerifyPayload{true, localHex}.toJson());
        emit logMessage(QStringLiteral("校验一致，文件已保存：%1").arg(m_finalPath));
    } else {
        m_sink.abort(); // 校验失败不落盘（设计文档 §12）
        setState(TransferState::VerifyFailed);
        sendControl(MessageType::FileVerify, FileVerifyPayload{false, localHex}.toJson());
        emit logMessage(QStringLiteral("SHA-256 不一致，已丢弃临时文件"));
    }
    finishSession();
}

void TransferSession::handleFileVerify(const QJsonObject& obj)
{
    if (m_role != TransferRole::Sender || m_state != TransferState::Verifying)
        return;
    m_idleTimer.stop();
    const FileVerifyPayload verify = FileVerifyPayload::fromJson(obj);

    if (verify.success) {
        setState(TransferState::Completed);
        emit logMessage(QStringLiteral("对端校验一致，传输完成"));
    } else {
        setState(TransferState::VerifyFailed);
        m_errorCode = ErrorCode::VerifyFailed;
        m_errorText = QStringLiteral("对端 SHA-256 校验失败");
        emit logMessage(m_errorText);
    }
    finishSession();
}

void TransferSession::handleCancel(const QJsonObject& obj)
{
    const CancelPayload cancel = CancelPayload::fromJson(obj);
    Q_UNUSED(cancel);
    fail(ErrorCode::Cancelled, QStringLiteral("对端取消了传输"), false);
}

void TransferSession::handleError(const QJsonObject& obj)
{
    const ErrorPayload err = ErrorPayload::fromJson(obj);
    fail(static_cast<ErrorCode>(err.code),
         err.message.isEmpty() ? errorMessage(static_cast<ErrorCode>(err.code)) : err.message,
         false);
}

// ---------------------------------------------------------------------------
// 通用
// ---------------------------------------------------------------------------
void TransferSession::onTimeout()
{
    fail(ErrorCode::Timeout, QStringLiteral("操作超时"), true);
}

void TransferSession::onSocketDisconnected()
{
    if (m_closing)
        return;
    if (isTransferActive(m_state))
        fail(ErrorCode::Disconnected, QStringLiteral("连接断开"), false);
}

void TransferSession::setState(TransferState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void TransferSession::fail(ErrorCode code, const QString& text, bool notifyPeer)
{
    if (m_finishedEmitted)
        return;
    m_errorCode = code;
    m_errorText = text;

    // 传输中失败：删除未完成的临时文件（设计文档 §11/§12）
    if (m_sink.isOpen() || !m_sink.tempPath().isEmpty())
        m_sink.abort();

    if (notifyPeer && !m_closing)
        sendErrorToPeer(code, text);

    switch (code) {
    case ErrorCode::Cancelled:   setState(TransferState::Cancelled);   break;
    case ErrorCode::UserRejected:setState(TransferState::Cancelled);   break;
    case ErrorCode::Timeout:     setState(TransferState::Failed);      break;
    case ErrorCode::VerifyFailed:setState(TransferState::VerifyFailed);break;
    case ErrorCode::PinMismatch: setState(TransferState::Failed);      break;
    default:                     setState(TransferState::Failed);      break;
    }

    emit logMessage(text);
    finishSession();
}

void TransferSession::finishSession()
{
    if (m_closing)
        return;
    m_closing = true;
    m_idleTimer.stop();
    closeGracefully();
    if (!m_finishedEmitted) {
        m_finishedEmitted = true;
        emit finished(m_state == TransferState::Completed);
    }
}

void TransferSession::closeGracefully()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        // 先把缓冲的帧（FILE_VERIFY/SESSION_CLOSE/ERROR 等）推给内核，再优雅断开
        m_socket->flush();
        m_socket->disconnectFromHost();
    } else if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
}

void TransferSession::resetIdleTimer(int ms)
{
    m_idleTimer.start(ms);
}

void TransferSession::emitProgress(bool force)
{
    if (force || m_progressClock.elapsed() >= PROGRESS_INTERVAL_MS) {
        m_progressClock.restart();
        emit progressChanged(transferredBytes(), m_fileSize);
    }
}

} // namespace lantransfer
