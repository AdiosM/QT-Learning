#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <memory>

#include "device/device.h"
#include "device/devicemanager.h"
#include "transfertypes.h"
#include "transfersession.h"
#include "transport/tcpclient.h"
#include "transport/tcpserver.h"

// ============================================================================
// TransferManager：传输任务队列与统计（设计文档 §16/§19）
//   - 发送：内部以 Client 角色建立 TCP 连接并创建 TransferSession
//   - 接收：TcpServer 移交的入站连接转为 TransferSession
//   - 单设备最多 1 条活动任务（V1.0）
//   - 400ms 周期刷新速度/剩余时间（EMA 平滑）
//   - TransferTaskModel（QAbstractListModel）供 Widgets 与 QML 共用
// ============================================================================
namespace lantransfer {

class TransferTaskModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        TaskIdRole = Qt::UserRole + 1,
        DeviceIdRole,
        DeviceNameRole,
        FileNameRole,
        FileSizeRole,
        RoleRole,          // TransferRole -> int
        StateRole,          // TransferState -> int
        StateTextRole,
        ProgressRole,       // 0-1000（千分比，QML ProgressBar 直接可用）
        TransferredRole,    // quint64
        SpeedRole,          // double bytes/s
        EtaSecondsRole,     // qint64，-1 未知
        ErrorTextRole,
        FinalPathRole,
    };
    Q_ENUM(Role)

    explicit TransferTaskModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void upsertTask(const TransferTask& task);
    const TransferTask* taskAt(int row) const;
    int indexOf(const QUuid& taskId) const;

private:
    QVector<TransferTask> m_tasks;
};

class TransferManager : public QObject
{
    Q_OBJECT
public:
    explicit TransferManager(DeviceManager* devices, QObject* parent = nullptr);
    ~TransferManager() override;

    // ---- TCP Server（接收方角色）----
    bool startServer(quint16 port = DEFAULT_TCP_PORT);
    void stopServer();
    bool isServerListening() const;
    quint16 serverPort() const;
    QString serverError() const;

    // ---- 接收目录 ----
    void setReceiveDir(const QString& dir);
    QString receiveDir() const;

    // ---- 发送（Client 角色）----
    // 返回任务 ID（taskId）；立即拒绝（文件 >500MB / 该设备已有活动任务）时返回空 QUuid
    QUuid sendFile(const Device& device, std::shared_ptr<FileSource> source,
                   const QString& fileName, qint64 fileSize);

    // ---- 取消 ----
    void cancelTransfer(const QUuid& taskId);

    // ---- 查询 ----
    TransferTaskModel* taskModel() { return &m_model; }
    int activeSessionCount() const;
    bool isDeviceBusy(const QString& deviceId) const;

signals:
    void logMessage(const QString& text);
    // 新会话建立（UI 由此挂接确认/冲突对话框）
    void sessionCreated(lantransfer::TransferSession* session);
    // 会话终态（session 即将 deleteLater，UI 需断开引用）
    void sessionFinished(lantransfer::TransferSession* session, bool success);
    // 入站传输请求（接收方 UI 弹出确认对话框）
    void pairRequestReceived(lantransfer::TransferSession* session);

private slots:
    void onStatsTick();

private:
    void attachSession(TransferSession* session, const QUuid& taskId);
    void createIncomingSession(QTcpSocket* socket);
    template <typename Fn> void updateTask(const QUuid& taskId, Fn&& fn);
    void finishTask(const QUuid& taskId, bool success);
    void addLog(const QString& text);
    bool checkDeviceBusy(TransferSession* session);

    DeviceManager* m_devices = nullptr;
    TcpServer m_server;
    QString m_receiveDir;
    TransferTaskModel m_model;
    QHash<QUuid, TransferSession*> m_sessions; // taskId -> session
    QVector<QPointer<TcpClient>> m_pendingClients;
    QTimer m_statsTimer;
    // 速度统计辅助
    struct SpeedStat {
        quint64 lastBytes = 0;
        qint64 lastTickMs = 0;
        double speed = 0.0;
    };
    QHash<QUuid, SpeedStat> m_speedStats;
};

} // namespace lantransfer
