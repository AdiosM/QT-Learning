#include "mainwindow.h"
#include "serialportmanager.h"   // ★ 在 .cpp 里才包含完整定义
#include "protocolparser.h"
#include "Modbusmanager.h"
#include "qcustomplot.h"  //mainwindow.h没有引入，在这里才真正引入
#include <QMessageBox>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QRandomGenerator>  // Qt 6 专用的随机数类


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("串口调试助手");
    resize(700, 500);

    // 创建通信层对象
    m_serialMgr = new SerialPortManager();//不传this，让它成为无 parent 的孤立对象

    // ★ 2. 创建工作线程
    m_serialThread = new QThread(this); // 线程对象本身属于 UI 线程，所以可以传 this
    m_serialMgr->moveToThread(m_serialThread);// ★ 3. 把通信层“扔”到工作线程里。m_serialMgr堆对象(new出来的)，被工作线程托管

    // ★ m_serialThread线程结束时，自动销毁 manager，防止内存泄漏（标准做法）
    connect(m_serialThread, &QThread::finished, m_serialMgr, &QObject::deleteLater);
    m_serialThread->start();// ★  启动线程！真的启动了，不是“登记”
    /*线程启动，
     * 启动①（系统线程）：start() 同步、立即、真实。不依赖任何东西。
     *启动②（子线程内事件循环，不是主线程UI的事件循环）：run() 里的默认 exec()。它才像定时器那样是 "事件循环驱动" 的 ——新线程能不能处理投递过来的信号/槽，取决于②这个事件循环在不在跑。
     */

    m_parser = new ProtocolParser(this); //有parent=this，因为它留在UI线程，不移线程

    // 初始化 ModbusManager
    m_modbusMgr = new ModbusManager(this);
    connect(m_modbusMgr, &ModbusManager::connectionStateChanged, this, &MainWindow::onModbusConnectionStateChanged);
    connect(m_modbusMgr, &ModbusManager::readDataReady, this, &MainWindow::onModbusReadDataReady);
    connect(m_modbusMgr, &ModbusManager::writeDataReady, this, &MainWindow::onModbusWriteDataReady);
    connect(m_modbusMgr, &ModbusManager::errorOccurred, this, &MainWindow::onModbusError);

    initUI();
    detectSerialPorts();//刷新串口下拉框

    // ★ UI只订阅通信层的"事件"，不碰 QSerialPort 本身
    connect(m_serialMgr, &SerialPortManager::portOpened,
            this, &MainWindow::onPortOpened);//m_serialMgr发送portOpened信号，接收着是当前Mainwindow对象
    connect(m_serialMgr, &SerialPortManager::portClosed,
            this, &MainWindow::onPortClosed);
    connect(m_serialMgr, &SerialPortManager::dataReceived,
            this, &MainWindow::onSerialDataReceived);
    connect(m_serialMgr, &SerialPortManager::serialError,
            this, &MainWindow::onSerialError);

    //协议层事件连接
    connect(m_parser, &ProtocolParser::frameReceived,
               this, &MainWindow::onFrameReceived);
    connect(m_parser, &ProtocolParser::syncLost,
               this, &MainWindow::onSyncLost);
//connect 做的不是 "在构造函数里调用一次函数"，而是 "向 Qt 系统注册一条永远生效的关系"
//构造函数return后，只是栈帧销毁，但连接表里的记录还在。
//事件循环运行时,构造函数中的这些connect函数完全不需要经过构造函数，只要在连接表里的记录找对于的信息就行
//connect 只关心发送者 / 接收者对象是否活着，不关心它是栈上还是堆上。
/*要调用connect(m_serialMgr, &SerialPortManager::portOpened, this, &MainWindow::onPortOpened);
m_serialMgr 必须指向一个已创建、还活着的 SerialPortManager
this（MainWindow）必须指向一个已创建、还活着的 MainWindow。
*/

/*信号发出来时，Qt凭什么能成功调到构造函数里connect过的哪个槽？
* 事件循环不会去 "调用 connect"——connect 在构造函数里早就执行完了（登记完成）。
* 事件循环触发的是 connect 登记的那些 "槽函数"，而触发它们的真正 "扳机" 是信号被发射。
* 以 connect(m_serialMgr, portOpened, this, onPortOpened) 为例，要真正触发，必须同时满足下面全部条件：
* 条件① 发送者对象活着  → m_serialMgr 存在（没被销毁）
* 条件② 接收者对象活着  → this(MainWindow) 存在
* 条件③ 连接还在        → 没有被 disconnect，且双方活着（自动断连就没触发）
* 条件④ 信号被发射      → manager 里真正执行了 emit portOpened(...)   ←★最关键
* 条件⑤ (跨线程时)事件循环在跑 → 队列信号等 UI 线程有空才被取出来执行
*/

    // ===== ★ 新增：跨线程安全加固 =====
    // 1. 将UI的发送请求，跨线程投递给工作线程的 manager
    // Qt::QueuedConnection 确保数据被拷贝到工作线程的事件队列中，绝对安全！
    connect(this, &MainWindow::requestSendData,
             m_serialMgr, &SerialPortManager::sendData,
            Qt::QueuedConnection);

    //初始化波形模拟定时器
    m_dataTimer = new QTimer(this);
    m_modbusPollTimer = new QTimer(this);

    m_lastTime = 0;
    connect(m_dataTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    m_dataTimer->start(50); //是把一个 50ms 的 "闹钟"注册进Qt的定时器机制，每50ms产生一个数据点，每过 50ms，QTimer发一次timeout信号
    //真正让闹钟每 50ms 响一次的，是 exec() 启动的事件循环。构造函数里 start(50) —— 只完成"登记闹钟"

    connect(m_modbusPollTimer, &QTimer::timeout, this, &MainWindow::onModbusPollTick);

}

MainWindow::~MainWindow()
{
    // 1. 如果串口还开着，先让 manager 关闭它
        if (m_serialMgr && m_serialMgr->isOpen()) {
            m_serialMgr->closePort();
        }

        // 2. 请求线程退出事件循环
        if (m_serialThread && m_serialThread->isRunning()) {
            m_serialThread->quit();

            // 3. 等待线程真正结束（最多等 3 秒，防止死锁）
            if (!m_serialThread->wait(3000)) {
                // 如果 3 秒还没停，强制终止（下策，但能保命）
                m_serialThread->terminate();
                m_serialThread->wait();
            }
        }
}

void MainWindow::initUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // ---------- 顶部：串口配置 ----------
    QGroupBox *configGroup = new QGroupBox("串口配置");//一个带边框和标题的容器
    QHBoxLayout *configLayout = new QHBoxLayout(configGroup);//水平布局，父对象为configGroup。布局不能脱离容器单独存在
    //布局有两种挂载方式:
    //1.布局挂在某个widget上，布局不是控件，必须指定它服务于哪个容器。布局必须传一个 widget当parent
    //2.布局也可以挂在另一个布局上，成为子布局

    configLayout->addWidget(new QLabel("串口:"));
    m_portComboBox = new QComboBox();
    m_portComboBox->setMinimumWidth(150);
    configLayout->addWidget(m_portComboBox);

    configLayout->addWidget(new QLabel("波特率:"));
    m_baudComboBox = new QComboBox();
    m_baudComboBox->addItems({"9600", "19200", "38400", "57600", "115200"});
    m_baudComboBox->setCurrentText("115200");
    configLayout->addWidget(m_baudComboBox);

    m_openCloseButton = new QPushButton("打开串口");
    m_openCloseButton->setFixedWidth(100);
    configLayout->addWidget(m_openCloseButton);
    configLayout->addStretch();

    // ---------- 中间：接收区 ----------
    QGroupBox *recvGroup = new QGroupBox("接收区");
    QVBoxLayout *recvMainLayout = new QVBoxLayout(recvGroup);

    QHBoxLayout *recvCtrlLayout = new QHBoxLayout();//注意这里没有widget父对象。因为recvGroup已经是recvMainLayout的父对象了，一个widget只能有一个主布局
    m_hexRecvCheckBox = new QCheckBox("HEX显示");
    m_clearRecvButton = new QPushButton("清空接收");
    m_clearRecvButton->setFixedWidth(80);

    recvCtrlLayout->addWidget(m_hexRecvCheckBox);
    recvCtrlLayout->addStretch();
    recvCtrlLayout->addWidget(m_clearRecvButton);
    recvMainLayout->addLayout(recvCtrlLayout);//recvCtrlLayout挂在recvMainLayout布局上，变成它的子布局

    m_parseCheckBox = new QCheckBox("协议解析");
    recvCtrlLayout->addWidget(m_parseCheckBox);   // 加在 m_hexRecvCheckBox 后面

    m_autoReplyCheckBox = new QCheckBox("自动应答");
    recvCtrlLayout->addWidget(m_autoReplyCheckBox);

    m_recvTextEdit = new QTextEdit();
    m_recvTextEdit->setReadOnly(true);
    m_recvTextEdit->setFont(QFont("Consolas", 10));
    recvMainLayout->addWidget(m_recvTextEdit);

    // ---------- 底部：发送区 ----------
    QGroupBox *sendGroup = new QGroupBox("发送区");

    QHBoxLayout *sendLayout = new QHBoxLayout(sendGroup);
    m_sendLineEdit = new QLineEdit();
    m_sendLineEdit->setPlaceholderText("输入文本或HEX(如: AA 01 FF)...");

    m_hexSendCheckBox = new QCheckBox("HEX发送");
    m_sendButton = new QPushButton("发送");
    m_sendButton->setFixedWidth(80);

    m_buildFrameCheckBox = new QCheckBox("协议组帧");

    sendLayout->addWidget(m_sendLineEdit);
    sendLayout->addWidget(m_hexSendCheckBox);
    sendLayout->addWidget(m_sendButton);
    sendLayout->addWidget(m_buildFrameCheckBox); // 放在 HEX发送 旁边

    // -----------Modbus 调试区-------------------
    QGroupBox *modbusGroup = new QGroupBox("Modbus RTU 调试区");
    QGridLayout *modbusLayout = new QGridLayout(modbusGroup); // 用网格布局更整齐

        // 第一行：从站地址、功能码、连接按钮
    modbusLayout->addWidget(new QLabel("从站地址:"), 0, 0);
    m_slaveIdSpin = new QSpinBox();
    m_slaveIdSpin->setRange(1, 247);
    m_slaveIdSpin->setValue(1);
    modbusLayout->addWidget(m_slaveIdSpin, 0, 1);

    modbusLayout->addWidget(new QLabel("功能码:"), 0, 2);
    m_funcCodeCombo = new QComboBox();
    m_funcCodeCombo->addItem("03 (读保持寄存器)", 3);
    m_funcCodeCombo->addItem("06 (写单个寄存器)", 6);
    modbusLayout->addWidget(m_funcCodeCombo, 0, 3);

    m_modbusConnectBtn = new QPushButton("连接 Modbus");
    m_modbusConnectBtn->setFixedWidth(120);
    modbusLayout->addWidget(m_modbusConnectBtn, 0, 4);

    m_autoPollCheckBox = new QCheckBox("自动轮询(500ms)");//自动轮询开关
    modbusLayout->addWidget(m_autoPollCheckBox, 0, 5);

    // 第二行：起始地址、数量/值、读/写按钮
    modbusLayout->addWidget(new QLabel("起始地址:"), 1, 0);
    m_startAddrSpin = new QSpinBox();
    m_startAddrSpin->setRange(0, 65535);
    m_startAddrSpin->setValue(0);
    modbusLayout->addWidget(m_startAddrSpin, 1, 1);

    modbusLayout->addWidget(new QLabel("数量/值:"), 1, 2);
    m_regCountSpin = new QSpinBox(); // 读的时候用
    m_regCountSpin->setRange(1, 125);
    m_regCountSpin->setValue(1);
    modbusLayout->addWidget(m_regCountSpin, 1, 3);

    m_regValueSpin = new QSpinBox(); // 写的时候用
    m_regValueSpin->setRange(0, 65535);
    m_regValueSpin->setValue(0);
    m_regValueSpin->hide(); // 默认隐藏，写的时候再显示（这里简化处理，先都显示）
    modbusLayout->addWidget(m_regValueSpin, 1, 3); // 覆盖位置

    m_modbusReadBtn = new QPushButton("读");
    m_modbusReadBtn->setFixedWidth(60);
    modbusLayout->addWidget(m_modbusReadBtn, 1, 4);

    m_modbusWriteBtn = new QPushButton("写");
    m_modbusWriteBtn->setFixedWidth(60);
    modbusLayout->addWidget(m_modbusWriteBtn, 1, 5);

    // 第三行：结果显示
    modbusLayout->addWidget(new QLabel("结果:"), 2, 0);
    m_modbusResultEdit = new QLineEdit();
    m_modbusResultEdit->setReadOnly(true);
    modbusLayout->addWidget(m_modbusResultEdit, 2, 1, 1, 5); // 横跨 5 列

    // ---------- 总布局 ----------
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(configGroup);
    mainLayout->addWidget(recvGroup, 1);
    mainLayout->addWidget(sendGroup);
    mainLayout->addWidget(modbusGroup);


    // ★ ----------波形图区域-----------------
    QGroupBox *plotGroup = new QGroupBox("实时波形 (模拟数据)");
    QVBoxLayout *plotLayout = new QVBoxLayout(plotGroup);

    m_customPlot = new QCustomPlot();
    m_customPlot->setMinimumHeight(200); // 给波形图一个最小高度

    // 配置坐标轴
    m_customPlot->xAxis->setLabel("时间 (s)");
    m_customPlot->yAxis->setLabel("数值");
    m_customPlot->xAxis->setRange(0, 10); // X轴初始范围 0-10秒
    m_customPlot->yAxis->setRange(-2, 2); // Y轴初始范围 -2 到 2

    // 添加一条曲线 (graph 0),蓝色线条，自定义模拟数据
    m_customPlot->addGraph();
    m_customPlot->graph(0)->setPen(QPen(Qt::blue)); // 蓝色线条
    m_customPlot->graph(0)->setName("自定义协议/模拟");

    //第二条曲线 (红色)，用于显示 Modbus 数据
    m_customPlot->addGraph();
    m_customPlot->graph(1)->setPen(QPen(Qt::red));
    m_customPlot->graph(1)->setName("Modbus RTU 寄存器");

    // 显示图例
    m_customPlot->legend->setVisible(true);
    m_customPlot->legend->setFont(QFont("Helvetica", 9));

    plotLayout->addWidget(m_customPlot);
    mainLayout->addWidget(plotGroup, 1); // 波形图也占据较多空间

    // ---------- 底部状态栏：收发计数 ----------
    m_countLabel = new QLabel("接收: 0 字节 | 发送: 0 字节");
    QPushButton *clearCountBtn = new QPushButton("清零");
    clearCountBtn->setFixedWidth(60);

    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->addWidget(m_countLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(clearCountBtn);
    mainLayout->addLayout(statusLayout);


    // ---------- 信号槽连接 ----------
    //和控件有关的connect，控件必须先生成，然后再连接，因为控件是在UI函数中生成，所以和控件有关的连接也在这里
    //而m_serialMgr等对象是在构造函数中生成，所以那些和它们有关的connect，也在构造函数中
    connect(m_openCloseButton, &QPushButton::clicked, this, &MainWindow::onOpenCloseButtonClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(m_clearRecvButton, &QPushButton::clicked, this, &MainWindow::onClearRecvClicked);
    connect(clearCountBtn, &QPushButton::clicked, this, &MainWindow::onClearCountClicked);

    connect(m_modbusConnectBtn, &QPushButton::clicked, this, &MainWindow::onModbusConnectClicked);
    connect(m_modbusReadBtn, &QPushButton::clicked, this, &MainWindow::onModbusReadClicked);
    connect(m_modbusWriteBtn, &QPushButton::clicked, this, &MainWindow::onModbusWriteClicked);

}

// ========== 枚举串口：改为向通信层要数据 ==========
void MainWindow::detectSerialPorts()
{
    m_portComboBox->clear();//把上一次填的清理掉

    const QList<PortInfo> ports = SerialPortManager::enumeratePorts();//将检查到的串口信息存放到列表ports中

    if (ports.isEmpty()) {
        m_portComboBox->addItem(QStringLiteral("未检测到串口"));
        m_openCloseButton->setEnabled(false);
        return;
    }

    m_openCloseButton->setEnabled(true);

    for (const PortInfo &p : ports) {//遍历串口列表
        QString displayText = p.portName;
        if (!p.description.isEmpty()) {
            displayText += QStringLiteral(" (") + p.description + QStringLiteral(")");
        }
        m_portComboBox->addItem(displayText, p.portName);//将串口信息填入到下拉框中
    }
}

// ========== 打开/关闭：UI 只发"命令"，不管细节 ==========
void MainWindow::onOpenCloseButtonClicked()//点击打开串口后执行这个函数
{
    if (!m_isPortOpen) {
        QString portName = m_portComboBox->currentData().toString();
        if (portName.isEmpty()) {
            QMessageBox::warning(this, "提示", "没有可用的串口");
            return;
        }

        SerialConfig cfg;        // 填配置结构体
        cfg.portName = portName;
        cfg.baudRate = m_baudComboBox->currentText().toInt();
        // 其余字段用默认值 8-N-1-无流控

        m_serialMgr->openPort(cfg); // 打开串口，成败由 portOpened 信号反馈
    } else {
        m_serialMgr->closePort(); // 完成由 portClosed 信号反馈
    }
}

// ========== 通信层事件：打开结果 ==========
void MainWindow::onPortOpened(bool ok, const QString &portName, const QString &errMsg)
{
    if (!ok) {
        QMessageBox::critical(this, "打开失败",
            "无法打开串口 " + portName + "\n\n" +
            "错误信息：" + errMsg + "\n\n" +
            "请检查：\n1. 串口是否被其他程序占用？\n2. 设备是否已连接？\n3. 驱动是否已安装？");
        return;
    }

    m_isPortOpen = true;   //同步状态
    m_parser->reset();   // 防止上一段连接的残留半帧污染新连接

    m_openCloseButton->setText("关闭串口");
    m_recvTextEdit->append(
        QDateTime::currentDateTime().toString("[hh:mm:ss] ") +
        "✅ 串口 " + portName + " 已打开");
    m_portComboBox->setEnabled(false);
    m_baudComboBox->setEnabled(false);
}

// ========== 通信层事件：已关闭 ==========
void MainWindow::onPortClosed()
{
    m_isPortOpen = false;  // ★ 新增：同步状态
    m_parser->reset();   //关闭立刻清空,防止上一段连接的残留半帧污染新连接

    m_openCloseButton->setText("打开串口");
    m_recvTextEdit->append(
        QDateTime::currentDateTime().toString("[hh:mm:ss] ") +
        "🔒 串口已关闭");
    m_portComboBox->setEnabled(true);
    m_baudComboBox->setEnabled(true);
    detectSerialPorts();   // 支持热插拔

    // ★ 核心修改：检查是否有人在等串口关闭去连 Modbus
    if (m_waitingForModbusConnect) {
        m_waitingForModbusConnect = false; // 重置标志位
        m_recvTextEdit->append("[Modbus] 串口已释放，正在连接 Modbus...");

        // 此时串口绝对已经释放了，直接连！
        QString portName = m_portComboBox->currentData().toString();
        int baudRate = m_baudComboBox->currentText().toInt();
        m_modbusMgr->connectDevice(portName, baudRate);
    }
}

// ========== 通信层事件：收到数据（原 onReadyRead 的内容） ==========
void MainWindow::onSerialDataReceived(const QByteArray &data)
{
    if (data.isEmpty()) return;

    m_recvCount += data.size();
    updateCountLabel();

    // ★ 协议解析模式：交给协议层，显示由 frameReceived 驱动
    if (m_parseCheckBox->isChecked()) {
        m_parser->appendData(data);
        return;
    }

    QString displayStr;
    if (m_hexRecvCheckBox->isChecked()) {
        displayStr = data.toHex(' ').toUpper();
    } else {
        displayStr = QString::fromLocal8Bit(data);
    }

    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
    m_recvTextEdit->append(timestamp + displayStr);
}

// ========== 通信层事件：运行时错误 ==========
void MainWindow::onSerialError(const QString &errMsg)
{
    m_recvTextEdit->append(
        QDateTime::currentDateTime().toString("[hh:mm:ss] ") +
        "⚠️ 串口错误: " + errMsg);
}

// ========== 发送（HEX 解析留在 UI 层，因为它是"显示格式"问题） ==========
void MainWindow::onSendButtonClicked()
{
    if (!m_isPortOpen) {//使用 UI 层的状态变量，不跨线程调用 m_serialMgr
        QMessageBox::warning(this, "提示", "请先打开串口！");
        return;
    }

    QString text = m_sendLineEdit->text();
    if (text.isEmpty()) return;

    QByteArray data;
    if (m_hexSendCheckBox->isChecked()) {
        data = hexStringToByteArray(text);
        if (data.isEmpty() && !text.trimmed().isEmpty()) {
            QMessageBox::warning(this, "格式错误",
                "HEX格式不正确!");
            return;
        }
    } else {
        data = text.toLocal8Bit();
    }

    // ★ 核心：如果勾选了协议组帧，把 data 当作载荷，套上帧头帧尾
    if (m_buildFrameCheckBox->isChecked()) {
        data = ProtocolParser::buildFrame(data);
    }
    // ★ 核心：不再直接调用 m_serialMgr->sendData()，而是发射信号！
    emit requestSendData(data);

    // 更新计数（因为变成了异步，这里直接按发送的字节数累加）
    m_sendCount += data.size();
    updateCountLabel();

    // 可选：在接收区回显发送的内容，方便调试
    m_recvTextEdit->append(
         QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ") +
        "📤 TX: " + QString(data.toHex(' ').toUpper()));
}

// ========== HEX 字符串转字节数组（"AA 01 FF" → 真实字节） ==========
QByteArray MainWindow::hexStringToByteArray(const QString &hexStr)
{
    QByteArray result;

    // 1. 移除所有空格，"AA 01" 和 "AA01" 都能识别
    QString cleanStr = hexStr.simplified().remove(" ");

    // 2. 长度必须是偶数（2 个字符 = 1 个字节）
    if (cleanStr.length() % 2 != 0) {
        return QByteArray();
    }

    // 3. 每两个字符截取一次，转成 16 进制字节
    for (int i = 0; i < cleanStr.length(); i += 2) {
        bool ok;
        quint8 byte = cleanStr.mid(i, 2).toUInt(&ok, 16);
        if (!ok) return QByteArray();   // 含非 HEX 字符（如 G、H）
        result.append(byte);
    }
    return result;
}

// ========== 清空接收区 ==========
void MainWindow::onClearRecvClicked()
{
    m_recvTextEdit->clear();
}

// ========== 清空计数器 ==========
void MainWindow::onClearCountClicked()
{
    m_recvCount = 0;
    m_sendCount = 0;
    updateCountLabel();
}

// ========== 更新计数标签 ==========
void MainWindow::updateCountLabel()
{
    m_countLabel->setText(QString("接收: %1 字节 | 发送: %2 字节")
        .arg(m_recvCount).arg(m_sendCount));
}

void MainWindow::onFrameReceived(const QByteArray &payload)
{
    m_recvTextEdit->append(
        QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ") +
        "📦 帧: " + QString(payload.toHex(' ').toUpper()));

    // ★ 新增：如果载荷长度 >= 2，提取前两个字节作为温度值
    if (payload.size() >= 2) {
        // 假设前两个字节是大端序的温度值（单位：0.1°C）
        // 例如：01 F4 = 500 = 50.0°C
        quint16 tempRaw = (quint8(payload.at(0)) << 8) | quint8(payload.at(1));
        double temperature = tempRaw / 10.0;  // 转换成摄氏度

        // 更新波形图
        addDataToPlot(temperature);
    }


    // ★ 自动应答逻辑
    if (m_autoReplyCheckBox->isChecked() && m_isPortOpen) {
           // 规则：收到 01，回复 02
        if (payload == QByteArray::fromHex("01")) {
             QByteArray replyPayload = QByteArray::fromHex("02");

               // 组帧并发送
            QByteArray replyFrame = ProtocolParser::buildFrame(replyPayload);
            emit requestSendData(replyFrame);

             m_recvTextEdit->append(
                QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ") +
                "🤖 自动应答: 02");
        }
    }
}

void MainWindow::onSyncLost(int droppedBytes)
{
    m_recvTextEdit->append(
        QDateTime::currentDateTime().toString("[hh:mm:ss] ") +
        QString("⚠️ 丢弃 %1 字节无效数据").arg(droppedBytes));
}

void MainWindow::onTimerTick()
{
    // 如果串口已打开，使用真实数据；否则使用模拟数据
    if (m_isPortOpen) {
         return;  // 串口打开时，由 onFrameReceived 负责更新波形
    }

    // 1. 生成模拟数据：正弦波 + 随机噪声
    double currentTime = m_lastTime + 0.05; // 50ms = 0.05s
    double value = qSin(currentTime * 2 * M_PI) + (QRandomGenerator::global()->bounded(100) - 50) / 100.0;
    // 公式：sin(2πt) + (-0.5 ~ 0.5) 的噪声

    // 2. 追加数据到数组
    m_timeData.append(currentTime);
    m_valueData.append(value);
    m_lastTime = currentTime;

    // 3. 保持数据窗口滑动（只显示最近 10 秒的数据）
    if (m_timeData.size() > 200) { // 200个点 * 50ms = 10秒
        m_timeData.removeFirst();
        m_valueData.removeFirst();
    }

    // 4. 更新曲线数据
    m_customPlot->graph(0)->setData(m_timeData, m_valueData);

    // 5. 调整 X 轴范围，实现"滚动"效果
    m_customPlot->xAxis->setRange(m_timeData.first(), m_timeData.last());

    // 6. 重绘！
    m_customPlot->replot();
}

void MainWindow::addDataToPlot(double value)
{
    // 1. 获取当前时间（X轴）
    double currentTime = m_lastTime + 0.05;  // 假设每帧间隔 50ms
    m_lastTime = currentTime;

    // 2. 追加数据
    m_timeData.append(currentTime);
    m_valueData.append(value);

    // 3. 保持滑动窗口（最近 10 秒）
    if (m_timeData.size() > 200) {
        m_timeData.removeFirst();
        m_valueData.removeFirst();
    }

    // 4. 更新曲线
    m_customPlot->graph(0)->setData(m_timeData, m_valueData);

    // 5. 调整 Y 轴范围（自适应）
    double minY = *std::min_element(m_valueData.begin(), m_valueData.end());
    double maxY = *std::max_element(m_valueData.begin(), m_valueData.end());
    double padding = (maxY - minY) * 0.1;  // 留 10% 边距
    if (padding < 1.0) padding = 1.0;
    m_customPlot->yAxis->setRange(minY - padding, maxY + padding);

    // 6. 调整 X 轴范围（滚动效果）
    m_customPlot->xAxis->setRange(m_timeData.first(), m_timeData.last());

    // 7. 重绘
    m_customPlot->replot();

    // 8. 在接收区显示温度值（可选）
    m_recvTextEdit->append(
        QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ") +
        QString("🌡️ 温度: %1 °C").arg(value, 0, 'f', 1));
}

// ========== Modbus 连接/断开 ==========
void MainWindow::onModbusConnectClicked()
{
    if (m_modbusMgr->isConnected()) {
        m_modbusMgr->disconnectDevice();
        m_modbusPollTimer->stop(); // 断开时停止轮询
    } else {
        // 连接前，必须先关闭普通串口（互斥）
        if (m_isPortOpen) {
            m_recvTextEdit->append("[Modbus] 已自动关闭普通串口");

            // ★ 核心修改：设置标志位，告诉 onPortClosed 稍后要连 Modbus
             m_waitingForModbusConnect = true;

             // 安全投递关闭请求到工作线程
            QMetaObject::invokeMethod(m_serialMgr, "closePort", Qt::QueuedConnection);
            return; // 注意：这里直接 return，不执行下面的连接代码
            }

        QString portName = m_portComboBox->currentData().toString();
        int baudRate = m_baudComboBox->currentText().toInt();
        m_modbusMgr->connectDevice(portName, baudRate);
    }
}

void MainWindow::onModbusConnectionStateChanged(bool connected, const QString &msg)
{
    m_modbusResultEdit->setText(msg);
    if (connected) {
        m_modbusConnectBtn->setText("断开 Modbus");
        m_recvTextEdit->append("[Modbus] " + msg);

        // ★ 核心修改 1：Modbus 连接成功，立刻停止模拟数据生成
        m_dataTimer->stop();

        // ★ 新增：如果勾选了自动轮询，启动定时器
        if (m_autoPollCheckBox->isChecked()) {
             m_modbusPollTimer->start(500); // 每 500ms 读一次
        }
    }
    else {
        m_modbusConnectBtn->setText("连接 Modbus");
        m_modbusPollTimer->stop();
        m_recvTextEdit->append("[Modbus] " + msg);

        // ★ 核心修改 2：Modbus 断开后，恢复模拟数据（让界面保持动态效果）
        m_dataTimer->start(50);
    }
}

void MainWindow::onModbusPollTick()
{
    if (!m_modbusMgr->isConnected()) return;

    // 默认读取地址 0 开始的 1 个寄存器（你可以根据需要修改）
    m_modbusMgr->readHoldingRegisters(m_startAddrSpin->value(), 1);
}

// ========== Modbus 读寄存器 ==========
void MainWindow::onModbusReadClicked()
{
    if (!m_modbusMgr->isConnected()) {
        QMessageBox::warning(this, "提示", "请先连接 Modbus 设备！");
        return;
    }
    int startAddr = m_startAddrSpin->value();
    int count = m_regCountSpin->value();
    m_modbusMgr->readHoldingRegisters(startAddr, count);
    m_recvTextEdit->append(QString("[Modbus] 发送读请求: 地址 %1, 数量 %2").arg(startAddr).arg(count));
}

void MainWindow::onModbusReadDataReady(const QVector<quint16> &data)
{
    if (data.isEmpty()) return;

       // 1. 提取第一个寄存器的值，并做简单转换（例如除以10变成浮点数，模拟温度）
    quint16 rawValue = data.first();
    double modbusValue = rawValue / 10.0;

    // 2. 更新结果显示
    m_modbusResultEdit->setText(QString("0x%1 (%2)").arg(rawValue, 4, 16, QChar('0')).toUpper().arg(modbusValue));
    m_recvTextEdit->append(QString("[Modbus] 收到: %1").arg(modbusValue));

    // 3. ★ 核心：将数据追加到红色曲线 (graph 1)
    // 为了简单，我们让 Modbus 曲线和蓝色曲线共用 X 轴时间，或者单独维护。
    // 这里我们直接复用 m_timeData 和 m_valueData 的逻辑，但只更新 graph(1)

    // 获取当前时间（如果和蓝色曲线共用时间轴）
    // 注意：如果蓝色曲线没在跑，m_lastTime 可能不更新，这里我们手动获取系统时间
    static double modbusTime = 0;
    modbusTime += 0.5; // 500ms = 0.5s

    // 为了演示清晰，我们为 Modbus 单独维护一组数据（避免和自定义协议数据混在一起）
    // 实际项目中可以封装成一个独立的 PlotManager 类
    static QVector<double> modbusTimeData;
    static QVector<double> modbusValueData;

    modbusTimeData.append(modbusTime);
    modbusValueData.append(modbusValue);

    if (modbusTimeData.size() > 50) { // 保留最近 25 秒的数据 (50 * 0.5s)
        modbusTimeData.removeFirst();
        modbusValueData.removeFirst();
    }

    m_customPlot->graph(1)->setData(modbusTimeData, modbusValueData);

    //Y轴自适应
    double allMinY = 2.0;
    double allMaxY = -2.0;

    if (!m_valueData.isEmpty()) {
        allMinY = qMin(allMinY, *std::min_element(m_valueData.begin(), m_valueData.end()));
        allMaxY = qMax(allMaxY, *std::max_element(m_valueData.begin(), m_valueData.end()));
    }
    if (!modbusValueData.isEmpty()) {
        allMinY = qMin(allMinY, *std::min_element(modbusValueData.begin(), modbusValueData.end()));
        allMaxY = qMax(allMaxY, *std::max_element(modbusValueData.begin(), modbusValueData.end()));
    }

    double padding = (allMaxY - allMinY) * 0.1;
    if (padding < 1.0) padding = 1.0;
    m_customPlot->yAxis->setRange(allMinY - padding, allMaxY + padding);


    // 调整 X 轴范围（取两条曲线中最大的时间范围）
    double maxTime = qMax(m_timeData.isEmpty() ? 0 : m_timeData.last(), modbusTime);
    m_customPlot->xAxis->setRange(qMax(0.0, maxTime - 10.0), maxTime); // 显示最近 10 秒

    m_customPlot->replot();
}

// ========== Modbus 写寄存器 ==========
void MainWindow::onModbusWriteClicked()
{
    if (!m_modbusMgr->isConnected()) {
        QMessageBox::warning(this, "提示", "请先连接 Modbus 设备！");
        return;
    }
    int address = m_startAddrSpin->value();
    quint16 value = m_regValueSpin->value();
    m_modbusMgr->writeSingleRegister(address, value);
    m_recvTextEdit->append(QString("[Modbus] 发送写请求: 地址 %1, 值 %2").arg(address).arg(value));
}

void MainWindow::onModbusWriteDataReady(int address, quint16 value)
{
    m_modbusResultEdit->setText(QString("写入成功: 地址 %1 = %2").arg(address).arg(value));
    m_recvTextEdit->append("[Modbus] 写入成功");
}

// ========== Modbus 错误处理 ==========
void MainWindow::onModbusError(const QString &errorMsg)
{
    m_modbusResultEdit->setText("错误: " + errorMsg);
    m_recvTextEdit->append("[Modbus] 错误: " + errorMsg);
}