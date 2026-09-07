#include "transfermanager.h"

#include <QDateTime>
#include <QStandardPaths>

namespace lantransfer {

namespace {
constexpr int STATS_INTERVAL_MS = 400;   // 进度/速度刷新（设计文档 §19：200-500ms）
constexpr double SPEED_EMA_ALPHA = 0.3;  // 速度平滑系数
} // namespace

// ---------------------------------------------------------------------------
// TransferTaskModel
// ---------------------------------------------------------------------------
TransferTaskModel::TransferTaskModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TransferTaskModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_tasks.size();
}

namespace {

QString taskSizeText(quint64 bytes)
{
    constexpr double KB = 1000.0;
    constexpr double MB = 1000.0 * KB;
    constexpr double GB = 1000.0 * MB;
    if (bytes >= GB)
        return QStringLiteral("%1 GB").arg(double(bytes) / GB, 0, 'f', 2);
    if (bytes >= MB)
        return QStringLiteral("%1 MB").arg(double(bytes) / MB, 0, 'f', 1);
    if (bytes >= KB)
        return QStringLiteral("%1 KB").arg(double(bytes) / KB, 0, 'f', 1);
    return QStringLiteral("%1 B").arg(bytes);
}

QString taskSpeedText(double bytesPerSec)
{
    constexpr double KB = 1000.0;
    constexpr double MB = 1000.0 * KB;
    if (bytesPerSec >= MB)
        return QStringLiteral("%1 MB/s").arg(bytesPerSec / MB, 0, 'f', 1);
    if (bytesPerSec >= KB)
        return QStringLiteral("%1 KB/s").arg(bytesPerSec / KB, 0, 'f', 0);
    if (bytesPerSec > 0)
        return QStringLiteral("%1 B/s").arg(bytesPerSec, 0, 'f', 0);
    return QStringLiteral("—");
}

QString taskEtaText(qint64 seconds)
{
    if (seconds < 0)
        return QStringLiteral("—");
    if (seconds < 60)
        return QStringLiteral("%1 秒").arg(seconds);
    if (seconds < 3600)
        return QStringLiteral("%1分%2秒").arg(seconds / 60).arg(seconds % 60);
    return QStringLiteral("%1时%2分").arg(seconds / 3600).arg((seconds % 3600) / 60);
}

} // namespace

QVariant TransferTaskModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size())
        return {};
    const TransferTask& t = m_tasks.at(index.row());
    switch (role) {
    case Qt::DisplayRole: {
        // 表格视图按列展示（QML 使用命名角色不受影响）
        switch (index.column()) {
        case 0: return t.fileName;
        case 1: return t.deviceName;
        case 2: return taskSizeText(t.fileSize);
        case 3: return t.role == TransferRole::Sender ? QStringLiteral("发送")
                                                      : QStringLiteral("接收");
        case 4: return transferStateText(t.state);
        case 5: return t.fileSize > 0
                ? QStringLiteral("%1%").arg(double(t.progress.transferredBytes) * 100.0 / double(t.fileSize), 0, 'f', 1)
                : QStringLiteral("100%");
        case 6: return taskSpeedText(t.progress.speedBytesPerSec);
        case 7: return taskEtaText(t.progress.remainingMs >= 0 ? t.progress.remainingMs / 1000 : -1);
        case 8: return t.errorText;
        }
        return {};
    }
    case Qt::ToolTipRole:
        return t.errorText.isEmpty() ? QStringLiteral("%1\n%2").arg(t.fileName, t.finalPath)
                                     : t.errorText;
    case FileNameRole:    return t.fileName;
    case TaskIdRole:      return t.taskId.toString(QUuid::WithoutBraces);
    case DeviceIdRole:    return t.deviceId;
    case DeviceNameRole:  return t.deviceName;
    case FileSizeRole:    return qulonglong(t.fileSize);
    case RoleRole:        return int(t.role);
    case StateRole:       return int(t.state);
    case StateTextRole:   return transferStateText(t.state);
    case ProgressRole:    return t.fileSize > 0 ? int(t.progress.transferredBytes * 1000 / t.fileSize) : 0;
    case TransferredRole: return qulonglong(t.progress.transferredBytes);
    case SpeedRole:       return t.progress.speedBytesPerSec;
    case EtaSecondsRole:  return t.progress.remainingMs >= 0 ? t.progress.remainingMs / 1000 : qint64(-1);
    case ErrorTextRole:   return t.errorText;
    case FinalPathRole:   return t.finalPath;
    }
    return {};
}

QHash<int, QByteArray> TransferTaskModel::roleNames() const
{
    return {
        {TaskIdRole, "taskId"},
        {DeviceIdRole, "deviceId"},
        {DeviceNameRole, "deviceName"},
        {FileNameRole, "fileName"},
        {FileSizeRole, "fileSize"},
        {RoleRole, "role"},
        {StateRole, "state"},
        {StateTextRole, "stateText"},
        {ProgressRole, "progress"},
        {TransferredRole, "transferred"},
        {SpeedRole, "speed"},
        {EtaSecondsRole, "etaSeconds"},
        {ErrorTextRole, "errorText"},
        {FinalPathRole, "finalPath"},
    };
}

void TransferTaskModel::upsertTask(const TransferTask& task)
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks.at(i).taskId == task.taskId) {
            m_tasks[i] = task;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx);
            return;
        }
    }
    beginInsertRows(QModelIndex(), m_tasks.size(), m_tasks.size());
    m_tasks.append(task);
    endInsertRows();
}

const TransferTask* TransferTaskModel::taskAt(int row) const
{
    if (row < 0 || row >= m_tasks.size())
        return nullptr;
    return &m_tasks.at(row);
}

int TransferTaskModel::indexOf(const QUuid& taskId) const
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks.at(i).taskId == taskId)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// TransferManager
// ---------------------------------------------------------------------------
TransferManager::TransferManager(DeviceManager* devices, QObject* parent)
    : QObject(parent)
    , m_devices(devices)
    , m_receiveDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
{
    connect(&m_server, &TcpServer::incomingConnection, this, &TransferManager::createIncomingSession);
    connect(&m_server, &TcpServer::listenError, this, &TransferManager::addLog);

    m_statsTimer.setInterval(STATS_INTERVAL_MS);
    connect(&m_statsTimer, &QTimer::timeout, this, &TransferManager::onStatsTick);
}

TransferManager::~TransferManager()
{
    m_statsTimer.stop();
    for (TransferSession* session : std::as_const(m_sessions))
        delete session;
    for (const QPointer<TcpClient>& client : std::as_const(m_pendingClients)) {
        if (client)
            delete client.data();
    }
}

bool TransferManager::startServer(quint16 port)
{
    const bool ok = m_server.start(port);
    if (ok)
        m_statsTimer.start();
    return ok;
}

void TransferManager::stopServer()
{
    m_server.stop();
    m_statsTimer.stop();
}

bool TransferManager::isServerListening() const
{
    return m_server.isListening();
}

quint16 TransferManager::serverPort() const
{
    return m_server.port();
}

QString TransferManager::serverError() const
{
    return m_server.errorString();
}

void TransferManager::setReceiveDir(const QString& dir)
{
    m_receiveDir = dir;
}

QString TransferManager::receiveDir() const
{
    return m_receiveDir;
}

QUuid TransferManager::sendFile(const Device& device, std::shared_ptr<FileSource> source,
                                const QString& fileName, qint64 fileSize)
{
    // 发送端第一道 500MB 校验（设计文档 §7.2）
    if (fileSize < 0) {
        addLog(QStringLiteral("文件不存在：%1").arg(fileName));
        return {};
    }
    if (quint64(fileSize) > MAX_FILE_SIZE) {
        addLog(QStringLiteral("拒绝发送：%1 超过 500 MB 限制").arg(fileName));
        return {};
    }
    // 单设备并发限制（设计文档 §10）
    if (isDeviceBusy(device.deviceId)) {
        addLog(QStringLiteral("与 %1 已有进行中的任务，请稍后再试").arg(device.name));
        return {};
    }

    // 先登记任务（Connecting 状态），连接成功后升级为会话
    const QUuid taskId = QUuid::createUuid();
    TransferTask task;
    task.taskId = taskId;
    task.deviceId = device.deviceId;
    task.deviceName = device.name;
    task.fileName = fileName;
    task.fileSize = quint64(fileSize);
    task.role = TransferRole::Sender;
    task.state = TransferState::Connecting;
    m_model.upsertTask(task);

    auto* client = new TcpClient(this);
    m_pendingClients.append(client);

    connect(client, &TcpClient::connected, this,
            [this, taskId, device, source, fileName, fileSize](QTcpSocket* socket) {
        TransferSession* session = new TransferSession(socket, this);
        session->setLocalInfo(m_devices->localDeviceId(), m_devices->localDeviceName(),
                              m_devices->localDeviceType(), serverPort(),
                              m_devices->appVersion());
        // 先确立角色再挂接（sessionCreated 监听方依赖 session->role() 区分收发）
        session->startAsSender(device.deviceId, source, fileName, fileSize);
        attachSession(session, taskId);
        updateTask(taskId, [&](TransferTask& t) { t.transferId = session->transferId(); });
        addLog(QStringLiteral("已连接 %1(%2)，开始发送 %3").arg(device.name, device.ip, fileName));
    });

    connect(client, &TcpClient::failed, this,
            [this, taskId, device](const QString& error) {
        updateTask(taskId, [&](TransferTask& t) {
            t.state = TransferState::Failed;
            t.errorCode = ErrorCode::Disconnected;
            t.errorText = error;
        });
        addLog(QStringLiteral("连接 %1(%2) 失败：%3").arg(device.name, device.ip, error));
    });

    // client 用完即弃（同时从登记表移除，避免悬垂指针）
    connect(client, &TcpClient::connected, client, [this, client](QTcpSocket*) {
        m_pendingClients.removeAll(client);
        client->deleteLater();
    });
    connect(client, &TcpClient::failed, client, [this, client](const QString&) {
        m_pendingClients.removeAll(client);
        client->deleteLater();
    });

    client->connectToHost(device.ip, device.tcpPort ? device.tcpPort : DEFAULT_TCP_PORT);
    return taskId;
}

void TransferManager::cancelTransfer(const QUuid& taskId)
{
    auto it = m_sessions.find(taskId);
    if (it != m_sessions.end())
        it.value()->userCancel();
}

int TransferManager::activeSessionCount() const
{
    int n = 0;
    for (TransferSession* s : m_sessions) {
        if (isTransferActive(s->state()))
            ++n;
    }
    return n;
}

bool TransferManager::isDeviceBusy(const QString& deviceId) const
{
    for (TransferSession* s : m_sessions) {
        if (s->peerDeviceId() == deviceId && isTransferActive(s->state()))
            return true;
    }
    return false;
}

void TransferManager::createIncomingSession(QTcpSocket* socket)
{
    const QUuid taskId = QUuid::createUuid();

    TransferTask task;
    task.taskId = taskId;
    task.deviceId = QStringLiteral("?");      // HELLO 后更新
    task.deviceName = QStringLiteral("未知设备");
    task.fileName = QStringLiteral("?");      // PAIR_REQUEST 后更新
    task.fileSize = 0;
    task.role = TransferRole::Receiver;
    task.state = TransferState::Handshaking;
    m_model.upsertTask(task);

    TransferSession* session = new TransferSession(socket, this);
    session->setLocalInfo(m_devices->localDeviceId(), m_devices->localDeviceName(),
                          m_devices->localDeviceType(), serverPort(),
                          m_devices->appVersion());
    // 先确立角色再挂接（与发送路径保持一致）
    session->startAsReceiver(m_receiveDir);
    attachSession(session, taskId);
}

void TransferManager::attachSession(TransferSession* session, const QUuid& taskId)
{
    m_sessions.insert(taskId, session);

    connect(session, &TransferSession::stateChanged, this,
            [this, taskId](TransferState state) {
        updateTask(taskId, [&](TransferTask& t) { t.state = state; });
    });

    connect(session, &TransferSession::progressChanged, this,
            [this, taskId](quint64 transferred, quint64 total) {
        updateTask(taskId, [&](TransferTask& t) {
            t.progress.transferredBytes = transferred;
            t.progress.totalBytes = total;
        });
    });

    connect(session, &TransferSession::finished, this,
            [this, taskId](bool success) { finishTask(taskId, success); });

    connect(session, &TransferSession::pairRequestReceived, this,
            [this, session](const QString& peerName, const QString& fileName,
                            quint64 fileSize, const QString&) {
        // 更新任务信息（HELLO/PAIR_REQUEST 后才知道对端身份与文件信息）
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (it.value() == session) {
                updateTask(it.key(), [&](TransferTask& t) {
                    t.deviceId = session->peerDeviceId();
                    t.deviceName = peerName.isEmpty() ? session->peerDeviceName() : peerName;
                    t.fileName = fileName;
                    t.fileSize = fileSize;
                });
                break;
            }
        }
        // 单设备并发限制：同设备已有活动会话则拒绝（Busy，设计文档 §10）
        if (checkDeviceBusy(session)) {
            session->rejectWithError(ErrorCode::Busy, QStringLiteral("对方设备正忙"));
            return;
        }
        emit pairRequestReceived(session);
    });

    connect(session, &TransferSession::logMessage, this, &TransferManager::addLog);
    emit sessionCreated(session);
}

bool TransferManager::checkDeviceBusy(TransferSession* session)
{
    for (TransferSession* other : m_sessions) {
        if (other == session)
            continue;
        if (other->peerDeviceId() == session->peerDeviceId()
            && isTransferActive(other->state())) {
            return true;
        }
    }
    return false;
}

template <typename Fn>
void TransferManager::updateTask(const QUuid& taskId, Fn&& fn)
{
    for (int i = 0; i < m_model.rowCount(); ++i) {
        const TransferTask* t = m_model.taskAt(i);
        if (t && t->taskId == taskId) {
            TransferTask updated = *t;
            fn(updated);
            m_model.upsertTask(updated);
            return;
        }
    }
}

void TransferManager::finishTask(const QUuid& taskId, bool success)
{
    auto it = m_sessions.find(taskId);
    if (it == m_sessions.end())
        return;
    TransferSession* session = it.value();

    updateTask(taskId, [&](TransferTask& t) {
        t.state = session->state();
        t.errorCode = session->errorCode();
        t.errorText = session->errorText();
        t.finalPath = session->finalPath();
    });

    m_sessions.erase(it);
    m_speedStats.remove(taskId);
    emit sessionFinished(session, success);
    session->deleteLater();
}

void TransferManager::onStatsTick()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        TransferSession* session = it.value();
        if (!isTransferActive(session->state()))
            continue;
        const QUuid taskId = it.key();
        SpeedStat& stat = m_speedStats[taskId];
        if (stat.lastTickMs == 0) {
            stat.lastTickMs = now;
            stat.lastBytes = session->transferredBytes();
            continue;
        }
        const qint64 dt = now - stat.lastTickMs;
        const quint64 bytes = session->transferredBytes();
        if (dt > 0) {
            const double instant = double(bytes - stat.lastBytes) * 1000.0 / double(dt);
            stat.speed = stat.speed > 0
                ? SPEED_EMA_ALPHA * instant + (1.0 - SPEED_EMA_ALPHA) * stat.speed
                : instant;
        }
        stat.lastBytes = bytes;
        stat.lastTickMs = now;

        updateTask(taskId, [&](TransferTask& t) {
            t.progress.transferredBytes = bytes;
            t.progress.speedBytesPerSec = stat.speed;
            t.progress.remainingMs = (stat.speed > 1.0 && session->fileSize() > bytes)
                ? qint64(double(session->fileSize() - bytes) / stat.speed * 1000.0)
                : qint64(-1);
        });
    }
}

void TransferManager::addLog(const QString& text)
{
    emit logMessage(QStringLiteral("[%1] %2")
                        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), text));
}

} // namespace lantransfer
