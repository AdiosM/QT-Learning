#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <memory>

#include "filesink.h"
#include "filesource.h"
#include "protocol/packetparser.h"
#include "security/hashcalculator.h"
#include "transfertypes.h"

// ============================================================================
// TransferSession：单任务会话状态机（设计文档 §6/§8/§11）
//   HELLO -> PAIR_REQUEST -> PAIR_RESPONSE(人工确认+PIN) -> FILE_INFO(双端
//   500MB 校验) -> FILE_ACCEPT -> FILE_DATA(1MiB 流式) -> FILE_END(携带发送端
//   SHA-256) -> FILE_VERIFY -> SESSION_CLOSE
// 发送方（Client 角色）与接收方（Server 角色）共用本类。
// 每任务一条 TCP 连接；会话结束时由持有者（TransferManager）销毁。
// ============================================================================
namespace lantransfer {

class TransferSession : public QObject
{
    Q_OBJECT
public:
    // 接管 socket（含所有权）；socket 必须已建立连接
    explicit TransferSession(QTcpSocket* socket, QObject* parent = nullptr);
    ~TransferSession() override;

    // 本端身份（HELLO 使用），由 TransferManager 在创建会话时注入
    void setLocalInfo(const QString& deviceId, const QString& deviceName,
                      const QString& deviceType, quint16 tcpPort,
                      const QString& appVersion);

    // ---- 角色初始化 ----
    // 发送方：连接建立后调用
    void startAsSender(const QString& peerDeviceId, std::shared_ptr<FileSource> source,
                       const QString& fileName, qint64 fileSize);
    // 接收方：TcpServer 移交连接后调用
    void startAsReceiver(const QString& receiveDir);

    // ---- 用户操作 ----
    void userAcceptPair(bool accepted);                       // 接收方：接受/拒绝
    void userConfirmPin(const QString& pin);                  // 发送方：输入对端屏幕显示的 PIN
    void userResolveConflict(ConflictResolution resolution);  // 接收方：同名文件处理
    void userCancel();                                        // 双方均可
    // 接收方在配对前主动拒绝（如单设备并发限制的 Busy）
    void rejectWithError(ErrorCode code, const QString& message);

    // ---- 查询 ----
    TransferState state() const { return m_state; }
    QUuid transferId() const { return m_transferId; }
    TransferRole role() const { return m_role; }
    QString peerDeviceId() const { return m_peerDeviceId; }
    QString peerDeviceName() const { return m_peerDeviceName; }
    QString peerIp() const;
    QString fileName() const { return m_fileName; }
    quint64 fileSize() const { return m_fileSize; }
    QString pairPin() const { return m_pin; } // 接收端生成的 6 位 PIN（UI 展示用）
    quint64 transferredBytes() const;
    QString finalPath() const { return m_finalPath; }
    QString errorText() const { return m_errorText; }
    ErrorCode errorCode() const { return m_errorCode; }

signals:
    void stateChanged(lantransfer::TransferState state);
    void logMessage(const QString& text);

    // 接收方：请求人工确认；pin 为接收端生成的 6 位 PIN（需展示给接收方用户，
    // 并口头/当面告知发送方用户输入）
    void pairRequestReceived(const QString& peerName, const QString& fileName,
                             quint64 fileSize, const QString& pin);
    // 发送方：需要用户输入对端屏幕显示的 PIN
    void pinRequired(const QString& pin);
    // 接收方：目标文件已存在，需要用户决策
    void fileConflict(const QString& fileName);

    // 进度（transferred/total）
    void progressChanged(quint64 transferred, quint64 total);

    // 终态信号（恰好一次）：success=false 时查看 errorText()/errorCode()
    void finished(bool success);

private:
    // ---- 帧分发 ----
    void onSocketReadyRead();
    void onSocketBytesWritten(qint64 bytes);
    void onSocketDisconnected();
    void dispatchPacket(const PacketParser::Packet& packet);

    // ---- 消息处理 ----
    void handleHello(const QJsonObject& obj);
    void handlePairRequest(const QJsonObject& obj);
    void handlePairResponse(const QJsonObject& obj);
    void handleFileInfo(const QJsonObject& obj);
    void handleFileAccept(const QJsonObject& obj);
    void handleFileData(const QByteArray& payload);
    void handleFileEnd(const QJsonObject& obj);
    void handleFileVerify(const QJsonObject& obj);
    void handleCancel(const QJsonObject& obj);
    void handleError(const QJsonObject& obj);

    // ---- 发送动作 ----
    void sendHello();
    void sendPairRequest();
    void sendFileInfo();
    void sendControl(MessageType type, const QJsonObject& payload);
    void sendErrorToPeer(ErrorCode code, const QString& message);
    void pumpFileData();

    // ---- 接收动作 ----
    void acceptFileInfo(const FileInfoPayload& info);

    // ---- 通用 ----
    void onTimeout();
    void setState(TransferState state);
    void fail(ErrorCode code, const QString& text, bool notifyPeer = true);
    void finishSession();
    void closeGracefully();
    void resetIdleTimer(int ms);
    void emitProgress(bool force = false);

private:
    QTcpSocket* m_socket = nullptr;
    PacketParser m_parser;
    QUuid m_transferId;                 // 首帧确立（发送方创建时生成）
    bool m_transferIdEstablished = false;

    // 本端身份
    QString m_localDeviceId;
    QString m_localDeviceName;
    QString m_localDeviceType;
    quint16 m_localTcpPort = 0;
    QString m_localAppVersion;

    TransferRole m_role = TransferRole::Receiver;
    TransferState m_state = TransferState::Idle;

    // 对端身份
    QString m_peerDeviceId;
    QString m_peerDeviceName;
    QString m_peerType;
    bool m_peerHelloReceived = false;

    // ---- 发送方状态 ----
    std::shared_ptr<FileSource> m_source;
    QString m_fileName;
    quint64 m_fileSize = 0;
    quint64 m_sentBytes = 0;
    bool m_eof = false;
    HashCalculator m_senderHash;
    QByteArray m_chunkBuf;
    QString m_expectedPin;      // PAIR_RESPONSE 携带的对端 PIN

    // ---- 接收方状态 ----
    QString m_receiveDir;
    FileSink m_sink;
    FileInfoPayload m_pendingFileInfo;  // 等待冲突决策时暂存
    ConflictPolicy m_conflictPolicy = ConflictPolicy::AutoRename;
    quint64 m_receivedBytes = 0;
    HashCalculator m_receiverHash;
    QString m_pin;              // 接收端生成的 6 位 PIN
    QString m_finalPath;

    // ---- 计时/进度 ----
    QTimer m_idleTimer;
    QElapsedTimer m_progressClock;

    // ---- 终态 ----
    ErrorCode m_errorCode = ErrorCode::Ok;
    QString m_errorText;
    bool m_finishedEmitted = false;
    bool m_closing = false;
};

} // namespace lantransfer
