#pragma once

#include <QHash>
#include <QMainWindow>

#include "device/devicemanager.h"
#include "discovery/discoverymanager.h"
#include "transfer/transfermanager.h"

class QLabel;
class QListView;
class QListWidget;
class QPlainTextEdit;
class QTableView;
class QPushButton;

// ============================================================================
// LanTransfer Windows 主窗口（设计文档 §15/§16）
//   左：设备列表（自动发现 + 手动连接）   右：发送面板 + 传输列表
//   底部：日志区；状态栏：本机 IP/端口
//   支持文件拖拽、500MB 限制展示、接收确认(PIN)、冲突处理
// ============================================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(lantransfer::DeviceManager* devices,
               lantransfer::TransferManager* transfers,
               lantransfer::DiscoveryManager* discovery,
               QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onManualConnect();
    void onChooseFiles();
    void onSendPending();
    void onChooseReceiveDir();
    void onCancelSelectedTask();
    void onDeviceViewDoubleClicked(const QModelIndex& index);

private:
    void setupUi();
    void setupMenus();
    void setupFirewallPrompt();
    void addPendingFiles(const QStringList& paths);
    void addPendingFile(const QString& path);
    void sendPendingToDevice(const lantransfer::Device& device);
    void handleIncomingPair(lantransfer::TransferSession* session);
    void handlePinRequired(lantransfer::TransferSession* session, const QString& pin);
    void handleFileConflict(lantransfer::TransferSession* session, const QString& fileName);
    void handleSessionFinished(lantransfer::TransferSession* session);
    void appendLog(const QString& text);
    void updateStatusBar();
    void refreshPendingMarkers();

    static QString formatSize(quint64 bytes);
    static QString formatSpeed(double bytesPerSec);
    static QString formatEta(qint64 seconds);

    lantransfer::DeviceManager* m_devices = nullptr;
    lantransfer::TransferManager* m_transfers = nullptr;
    lantransfer::DiscoveryManager* m_discovery = nullptr;

    // UI 控件
    QListView* m_deviceView = nullptr;
    QListWidget* m_pendingList = nullptr;
    QTableView* m_taskView = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_cancelTaskButton = nullptr;
    QLabel* m_statusIp = nullptr;
    QLabel* m_statusPorts = nullptr;

    // 会话对话框（会话结束时清理）
    QHash<lantransfer::TransferSession*, QWidget*> m_sessionDialogs;

    bool m_firewallPrompted = false;
};
