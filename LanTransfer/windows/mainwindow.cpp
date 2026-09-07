#include "mainwindow.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "dialogs.h"
#include "protocol/packet.h"

using namespace lantransfer;

namespace {

// 设备列表项：在线绿色圆点 + 名称 + IP/端口
class DeviceItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const bool online = index.data(DeviceModel::OnlineRole).toBool();
        const QString ip = index.data(DeviceModel::IpRole).toString();
        const int tcpPort = index.data(DeviceModel::TcpPortRole).toInt();

        opt.text = opt.text;
        painter->save();

        // 背景（选中态）
        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, opt.palette.highlight());
        }
        // 在线圆点
        painter->setBrush(online ? QColor(0x2e, 0xcc, 0x71) : QColor(0xaa, 0xaa, 0xaa));
        painter->setPen(Qt::NoPen);
        const QPointF center(opt.rect.left() + 14, opt.rect.center().y());
        painter->drawEllipse(center, 5, 5);

        // 名称
        QRect textRect = opt.rect.adjusted(28, 4, -8, 0);
        painter->setPen(opt.state & QStyle::State_Selected
                            ? opt.palette.highlightedText().color()
                            : opt.palette.text().color());
        QFont boldFont = opt.font;
        boldFont.setBold(true);
        painter->setFont(boldFont);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                          index.data(DeviceModel::NameRole).toString());

        // IP/端口（第二行小字）
        QFont smallFont = opt.font;
        smallFont.setPointSizeF(smallFont.pointSizeF() * 0.85);
        painter->setFont(smallFont);
        painter->setPen(opt.state & QStyle::State_Selected
                            ? opt.palette.highlightedText().color()
                            : QColor(0x88, 0x88, 0x88));
        QRect subRect = opt.rect.adjusted(28, opt.rect.height() / 2, -8, 0);
        painter->drawText(subRect, Qt::AlignLeft | Qt::AlignTop,
                          QStringLiteral("%1:%2 %3")
                              .arg(ip)
                              .arg(tcpPort)
                              .arg(online ? QStringLiteral("在线") : QStringLiteral("离线")));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        return QSize(option.rect.width(), 44);
    }
};

} // namespace

MainWindow::MainWindow(DeviceManager* devices, TransferManager* transfers,
                       DiscoveryManager* discovery, QWidget* parent)
    : QMainWindow(parent)
    , m_devices(devices)
    , m_transfers(transfers)
    , m_discovery(discovery)
{
    setupUi();
    setupMenus();

    // ---- 管理器信号 ----
    connect(m_transfers, &TransferManager::logMessage, this, &MainWindow::appendLog);
    connect(m_discovery, &DiscoveryManager::logMessage, this, &MainWindow::appendLog);

    // 入站传输请求 → 接收确认对话框
    connect(m_transfers, &TransferManager::pairRequestReceived, this,
            &MainWindow::handleIncomingPair);

    // 新会话 → 挂接 PIN 输入与文件冲突对话框
    connect(m_transfers, &TransferManager::sessionCreated, this,
            [this](TransferSession* session) {
        connect(session, &TransferSession::pinRequired, this,
                [this, session](const QString& pin) { handlePinRequired(session, pin); });
        connect(session, &TransferSession::fileConflict, this,
                [this, session](const QString& name) { handleFileConflict(session, name); });
        connect(session, &TransferSession::finished, this,
                [this, session](bool) { handleSessionFinished(session); });
    });

    // 手动连接入站（无配对请求时也要清理对话框）
    connect(m_transfers, &TransferManager::sessionFinished, this,
            [this](TransferSession* session, bool) { handleSessionFinished(session); });

    // 设备表变化 → 刷新视图
    connect(m_devices, &DeviceManager::devicesChanged, this, [this] {
        m_devices->model()->setDevices(m_devices->allDevices());
    });
    m_devices->model()->setDevices(m_devices->allDevices());

    // 状态栏定时刷新（发现/离线状态变化）
    auto* statusTimer = new QTimer(this);
    statusTimer->setInterval(2000);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    statusTimer->start();
    updateStatusBar();

    // 首次运行防火墙提示
    setupFirewallPrompt();

    setWindowTitle(QStringLiteral("LanTransfer 局域网文件互传"));
    resize(1080, 720);
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);

    // ---- 顶部工具栏 ----
    auto* toolbar = new QHBoxLayout;
    auto* manualBtn = new QPushButton(QStringLiteral("手动连接…"), central);
    connect(manualBtn, &QPushButton::clicked, this, &MainWindow::onManualConnect);
    toolbar->addWidget(manualBtn);
    auto* dirBtn = new QPushButton(QStringLiteral("接收目录…"), central);
    connect(dirBtn, &QPushButton::clicked, this, &MainWindow::onChooseReceiveDir);
    toolbar->addWidget(dirBtn);
    toolbar->addStretch();
    toolbar->addWidget(new QLabel(
        QStringLiteral("单文件最大 500 MB（500,000,000 字节）"), central));
    rootLayout->addLayout(toolbar);

    // ---- 主体：左右分栏 ----
    auto* splitter = new QSplitter(Qt::Horizontal, central);
    rootLayout->addWidget(splitter, 1);

    // 左侧：设备列表
    auto* devicePanel = new QWidget(splitter);
    auto* deviceLayout = new QVBoxLayout(devicePanel);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->addWidget(new QLabel(QStringLiteral("<b>局域网设备</b>"), devicePanel));
    m_deviceView = new QListView(devicePanel);
    m_deviceView->setModel(m_devices->model());
    m_deviceView->setItemDelegate(new DeviceItemDelegate(m_deviceView));
    m_deviceView->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_deviceView, &QListView::doubleClicked, this, &MainWindow::onDeviceViewDoubleClicked);
    deviceLayout->addWidget(m_deviceView, 1);

    // 右侧：发送面板 + 传输列表
    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // 发送面板
    auto* sendGroup = new QGroupBox(QStringLiteral("发送文件（可拖拽文件到窗口）"), rightPanel);
    auto* sendLayout = new QVBoxLayout(sendGroup);
    m_pendingList = new QListWidget(sendGroup);
    m_pendingList->setMaximumHeight(140);
    m_pendingList->setAcceptDrops(false);
    sendLayout->addWidget(m_pendingList);

    auto* sendButtons = new QHBoxLayout;
    auto* chooseBtn = new QPushButton(QStringLiteral("选择文件…"), sendGroup);
    connect(chooseBtn, &QPushButton::clicked, this, &MainWindow::onChooseFiles);
    sendButtons->addWidget(chooseBtn);
    auto* clearBtn = new QPushButton(QStringLiteral("清空列表"), sendGroup);
    connect(clearBtn, &QPushButton::clicked, this, [this] { m_pendingList->clear(); });
    sendButtons->addWidget(clearBtn);
    sendButtons->addStretch();
    m_sendButton = new QPushButton(QStringLiteral("发送到选中设备"), sendGroup);
    m_sendButton->setDefault(true);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendPending);
    sendButtons->addWidget(m_sendButton);
    sendLayout->addLayout(sendButtons);
    rightLayout->addWidget(sendGroup);

    // 传输列表
    auto* taskGroup = new QGroupBox(QStringLiteral("传输任务"), rightPanel);
    auto* taskLayout = new QVBoxLayout(taskGroup);
    m_taskView = new QTableView(taskGroup);
    m_taskView->setModel(m_transfers->taskModel());
    m_taskView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_taskView->verticalHeader()->hide();
    m_taskView->horizontalHeader()->setStretchLastSection(true);
    const QStringList headers = {
        QStringLiteral("文件名"), QStringLiteral("设备"), QStringLiteral("大小"),
        QStringLiteral("方向"), QStringLiteral("状态"), QStringLiteral("进度"),
        QStringLiteral("速度"), QStringLiteral("剩余"), QStringLiteral("说明"),
    };
    for (int i = 0; i < headers.size(); ++i)
        m_taskView->model()->setHeaderData(i, Qt::Horizontal, headers.at(i));
    m_taskView->setColumnWidth(0, 180);
    m_taskView->setColumnWidth(1, 110);
    m_taskView->setColumnWidth(2, 80);
    m_taskView->setColumnWidth(3, 50);
    m_taskView->setColumnWidth(4, 90);
    m_taskView->setColumnWidth(5, 70);
    m_taskView->setColumnWidth(6, 80);
    m_taskView->setColumnWidth(7, 70);
    taskLayout->addWidget(m_taskView, 1);

    auto* taskButtons = new QHBoxLayout;
    m_cancelTaskButton = new QPushButton(QStringLiteral("取消选中任务"), taskGroup);
    connect(m_cancelTaskButton, &QPushButton::clicked, this, &MainWindow::onCancelSelectedTask);
    taskButtons->addWidget(m_cancelTaskButton);
    taskButtons->addStretch();
    taskLayout->addLayout(taskButtons);
    rightLayout->addWidget(taskGroup, 1);

    // ---- 底部：日志 ----
    m_logView = new QPlainTextEdit(central);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);
    m_logView->setMaximumHeight(140);
    rootLayout->addWidget(m_logView);

    // ---- 状态栏 ----
    m_statusIp = new QLabel(this);
    m_statusPorts = new QLabel(this);
    statusBar()->addWidget(m_statusIp);
    statusBar()->addPermanentWidget(m_statusPorts);

    setAcceptDrops(true);
}

void MainWindow::setupMenus()
{
    QMenu* settingsMenu = menuBar()->addMenu(QStringLiteral("设置(&S)"));
    settingsMenu->addAction(QStringLiteral("接收目录…"), this, &MainWindow::onChooseReceiveDir);
    settingsMenu->addAction(QStringLiteral("手动连接…"), this, &MainWindow::onManualConnect);
    QMenu* helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->addAction(QStringLiteral("关于"), this, [this] {
        QMessageBox::about(this, QStringLiteral("关于 LanTransfer"),
                           QStringLiteral("<b>LanTransfer V%1</b><br>局域网文件互传工具（电脑 ↔ 电脑）<br><br>"
                                          "单文件上限：500,000,000 字节（显示为 500 MB）<br>"
                                          "传输完成后自动进行 SHA-256 完整性校验。<br>"
                                          "PIN 用于配对确认，不加密传输内容。")
                               .arg(QStringLiteral(LANTRANSFER_APP_VERSION)));
    });
}

void MainWindow::setupFirewallPrompt()
{
    QSettings settings;
    if (settings.value(QStringLiteral("firewall/prompted"), false).toBool())
        return;
    settings.setValue(QStringLiteral("firewall/prompted"), true);

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Windows 防火墙提示"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("LanTransfer 使用以下端口进行局域网通信："));
    box.setInformativeText(QStringLiteral("TCP %1：文件传输\nUDP %2：设备发现\n\n"
                                          "首次运行时 Windows 防火墙可能阻止通信，"
                                          "请允许本程序通过专用网络。")
                               .arg(m_transfers->serverPort())
                               .arg(m_discovery->udpPort()));
    auto* addRuleBtn = box.addButton(QStringLiteral("尝试自动添加防火墙规则"), QMessageBox::ActionRole);
    box.addButton(QStringLiteral("我知道了"), QMessageBox::AcceptRole);
    box.exec();

    if (box.clickedButton() == addRuleBtn) {
        const int tcpExit = QProcess::execute(QStringLiteral("netsh"),
                                              QStringList() << QStringLiteral("advfirewall")
                                                            << QStringLiteral("firewall")
                                                            << QStringLiteral("add") << QStringLiteral("rule")
                                                            << QStringLiteral("name=LanTransfer TCP")
                                                            << QStringLiteral("dir=in") << QStringLiteral("action=allow")
                                                            << QStringLiteral("protocol=TCP")
                                                            << QStringLiteral("localport=%1").arg(m_transfers->serverPort()));
        const int udpExit = QProcess::execute(QStringLiteral("netsh"),
                                              QStringList() << QStringLiteral("advfirewall")
                                                            << QStringLiteral("firewall")
                                                            << QStringLiteral("add") << QStringLiteral("rule")
                                                            << QStringLiteral("name=LanTransfer UDP")
                                                            << QStringLiteral("dir=in") << QStringLiteral("action=allow")
                                                            << QStringLiteral("protocol=UDP")
                                                            << QStringLiteral("localport=%1").arg(m_discovery->udpPort()));
        if (tcpExit == 0 && udpExit == 0) {
            QMessageBox::information(this, QStringLiteral("LanTransfer"),
                                     QStringLiteral("防火墙规则已添加。"));
        } else {
            QMessageBox::warning(this, QStringLiteral("LanTransfer"),
                                 QStringLiteral("自动添加失败（需要管理员权限）。请以管理员身份运行 netsh 或手动放行端口：\n\n"
                                                "netsh advfirewall firewall add rule name=\"LanTransfer TCP\" dir=in action=allow protocol=TCP localport=%1\n"
                                                "netsh advfirewall firewall add rule name=\"LanTransfer UDP\" dir=in action=allow protocol=UDP localport=%2")
                                     .arg(m_transfers->serverPort())
                                     .arg(m_discovery->udpPort()));
        }
    }
}

// ---------------------------------------------------------------------------
// 交互槽
// ---------------------------------------------------------------------------
void MainWindow::onManualConnect()
{
    ManualConnectDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString ip = dialog.ip();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("手动连接"), QStringLiteral("请输入 IP 地址。"));
        return;
    }
    const quint16 tcpPort = dialog.tcpPort();
    if (tcpPort == 0) {
        // 自动探测：先以默认端口登记，同时发 UDP 探测获取真实端口
        m_devices->upsertManualDevice(ip, DEFAULT_TCP_PORT);
        m_discovery->sendProbe(ip, m_discovery->udpPort());
        appendLog(QStringLiteral("已添加手动设备 %1，正在探测端口…").arg(ip));
    } else {
        m_devices->upsertManualDevice(ip, tcpPort);
        appendLog(QStringLiteral("已添加手动设备 %1:%2").arg(ip).arg(tcpPort));
    }
    m_devices->model()->setDevices(m_devices->allDevices());
}

void MainWindow::onChooseFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择要发送的文件"));
    if (!paths.isEmpty())
        addPendingFiles(paths);
}

void MainWindow::onSendPending()
{
    if (m_pendingList->count() == 0) {
        QMessageBox::information(this, QStringLiteral("发送文件"),
                                 QStringLiteral("请先添加要发送的文件。"));
        return;
    }
    const QModelIndex current = m_deviceView->currentIndex();
    if (!current.isValid()) {
        QMessageBox::information(this, QStringLiteral("发送文件"),
                                 QStringLiteral("请先在左侧选中一台设备。"));
        return;
    }
    const Device* device = m_devices->model()->deviceAt(current.row());
    if (!device) {
        QMessageBox::warning(this, QStringLiteral("发送文件"), QStringLiteral("设备列表已更新，请重新选择。"));
        return;
    }
    sendPendingToDevice(*device);
}

void MainWindow::sendPendingToDevice(const Device& device)
{
    // 复制设备信息（发送过程异步，模型可能更新）
    const Device target = device;

    QStringList failed;
    QList<QListWidgetItem*> toRemove;
    for (int i = 0; i < m_pendingList->count(); ++i) {
        QListWidgetItem* item = m_pendingList->item(i);
        const QString path = item->data(Qt::UserRole).toString();
        const QFileInfo info(path);
        if (!info.exists()) {
            failed << QStringLiteral("%1（文件不存在）").arg(info.fileName());
            toRemove << item;
            continue;
        }
        if (quint64(info.size()) > MAX_FILE_SIZE) {
            failed << QStringLiteral("%1（超过 500 MB 限制）").arg(info.fileName());
            toRemove << item;
            continue;
        }
        auto source = std::make_shared<QFileFileSource>(path);
        const QUuid taskId = m_transfers->sendFile(target, source, info.fileName(), info.size());
        if (taskId.isNull()) {
            failed << info.fileName();
        } else {
            toRemove << item;
        }
    }
    for (QListWidgetItem* item : std::as_const(toRemove))
        delete m_pendingList->takeItem(m_pendingList->row(item));

    if (!failed.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("发送文件"),
                             QStringLiteral("以下文件未加入发送队列：\n%1").arg(failed.join(QLatin1Char('\n'))));
    }
}

void MainWindow::onChooseReceiveDir()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择接收目录"), m_transfers->receiveDir());
    if (dir.isEmpty())
        return;
    m_transfers->setReceiveDir(dir);
    QSettings settings;
    settings.setValue(QStringLiteral("storage/receiveDir"), dir);
    appendLog(QStringLiteral("接收目录已更改为：%1").arg(dir));
}

void MainWindow::onCancelSelectedTask()
{
    const QModelIndex current = m_taskView->currentIndex();
    if (!current.isValid()) {
        QMessageBox::information(this, QStringLiteral("取消任务"),
                                 QStringLiteral("请先在传输列表中选择要取消的任务。"));
        return;
    }
    const TransferTask* task = m_transfers->taskModel()->taskAt(current.row());
    if (task)
        m_transfers->cancelTransfer(task->taskId);
}

void MainWindow::onDeviceViewDoubleClicked(const QModelIndex& index)
{
    Q_UNUSED(index);
    if (m_pendingList->count() > 0)
        onSendPending();
}

// ---------------------------------------------------------------------------
// 拖拽
// ---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QStringList paths;
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    if (!paths.isEmpty())
        addPendingFiles(paths);
    event->acceptProposedAction();
}

// ---------------------------------------------------------------------------
// 待发送列表
// ---------------------------------------------------------------------------
void MainWindow::addPendingFiles(const QStringList& paths)
{
    for (const QString& path : paths)
        addPendingFile(path);
}

void MainWindow::addPendingFile(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        appendLog(QStringLiteral("跳过不存在的文件：%1").arg(path));
        return;
    }
    auto* item = new QListWidgetItem(m_pendingList);
    item->setText(info.fileName());
    item->setToolTip(path);
    item->setData(Qt::UserRole, path);
    refreshPendingMarkers();
    appendLog(QStringLiteral("已添加待发送文件：%1（%2）").arg(info.fileName(), formatSize(quint64(info.size()))));
}

void MainWindow::refreshPendingMarkers()
{
    for (int i = 0; i < m_pendingList->count(); ++i) {
        QListWidgetItem* item = m_pendingList->item(i);
        const QString path = item->data(Qt::UserRole).toString();
        const QFileInfo info(path);
        const bool over = info.exists() && quint64(info.size()) > MAX_FILE_SIZE;
        item->setForeground(over ? QColor(0xd0, 0x30, 0x30) : QColor());
        if (over) {
            item->setText(QStringLiteral("%1（超过 500 MB 限制，不可发送）").arg(info.fileName()));
        } else {
            item->setText(QStringLiteral("%1（%2）").arg(info.fileName(), formatSize(quint64(info.size()))));
        }
    }
}

// ---------------------------------------------------------------------------
// 会话对话框
// ---------------------------------------------------------------------------
void MainWindow::handleIncomingPair(TransferSession* session)
{
    const QString pin = session->pairPin();
    auto* dialog = new IncomingPairDialog(session->peerDeviceName(), session->fileName(),
                                          session->fileSize(), pin, this);
    m_sessionDialogs.insert(session, dialog);
    connect(dialog, &IncomingPairDialog::acceptedPair, this,
            [session](bool accepted) { session->userAcceptPair(accepted); });
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::handlePinRequired(TransferSession* session, const QString& pin)
{
    Q_UNUSED(pin);
    auto* dialog = new PinInputDialog(session->peerDeviceName(), this);
    m_sessionDialogs.insert(session, dialog);
    connect(dialog, &PinInputDialog::pinEntered, this, [session](const QString& pinText) {
        if (!pinText.isEmpty())
            session->userConfirmPin(pinText);
        else
            session->userCancel();
    });
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::handleFileConflict(TransferSession* session, const QString& fileName)
{
    auto* dialog = new ConflictDialog(fileName, this);
    m_sessionDialogs.insert(session, dialog);
    connect(dialog, &ConflictDialog::resolved, this,
            [session](ConflictResolution r) { session->userResolveConflict(r); });
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::handleSessionFinished(TransferSession* session)
{
    QWidget* dialog = m_sessionDialogs.take(session);
    if (dialog)
        dialog->deleteLater();
}

// ---------------------------------------------------------------------------
// 杂项
// ---------------------------------------------------------------------------
void MainWindow::appendLog(const QString& text)
{
    m_logView->appendPlainText(text);
}

void MainWindow::updateStatusBar()
{
    const QList<QHostAddress> ips = DeviceManager::localIPv4Addresses();
    const QString ipText = ips.isEmpty() ? QStringLiteral("无可用局域网接口")
                                         : ips.first().toString();
    m_statusIp->setText(QStringLiteral("本机 IP：%1    设备名：%2")
                            .arg(ipText, m_devices->localDeviceName()));
    m_statusPorts->setText(QStringLiteral("TCP：%1    UDP：%2")
                               .arg(m_transfers->serverPort())
                               .arg(m_discovery->udpPort()));
}

QString MainWindow::formatSize(quint64 bytes)
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

QString MainWindow::formatSpeed(double bytesPerSec)
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

QString MainWindow::formatEta(qint64 seconds)
{
    if (seconds < 0)
        return QStringLiteral("—");
    if (seconds < 60)
        return QStringLiteral("%1 秒").arg(seconds);
    if (seconds < 3600)
        return QStringLiteral("%1 分 %2 秒").arg(seconds / 60).arg(seconds % 60);
    return QStringLiteral("%1 小时 %2 分").arg(seconds / 3600).arg((seconds % 3600) / 60);
}
