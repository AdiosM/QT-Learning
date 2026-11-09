#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include<QThread>

#include <QTimer>          // 定时器
#include <QVector>


class SerialPortManager;   // ★ 前置声明：头文件里只用到指针，不需要完整定义，这样可以不 include serialportmanager.h，减少编译依赖
class ProtocolParser;
class QCustomPlot;//告诉编译器“有个类叫 QCustomPlot”，但不告诉它里面有什么
class ModbusManager;
class QSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void requestSendData(const QByteArray &data); // ★跨线程发送请求信号
    //凡是触碰工作线程对象的行为，一律 "发信号"，绝不在 UI 线程直接调它的成员函数

private slots:
    void onOpenCloseButtonClicked();
    void onSendButtonClicked();
    void onClearRecvClicked();
    void onClearCountClicked();

    void onTimerTick();      //定时器触发，用于模拟数据
    void onModbusPollTick();   // ★ 新增：Modbus 定时轮询

    // ★ 新增：接收通信层事件的槽
    void onPortOpened(bool ok, const QString &portName, const QString &errMsg);
    void onPortClosed();
    void onSerialDataReceived(const QByteArray &data);
    void onSerialError(const QString &errMsg);

    void onFrameReceived(const QByteArray &payload);
    void onSyncLost(int droppedBytes);

    // ★ 新增：Modbus 相关槽函数
    void onModbusConnectClicked();
    void onModbusReadClicked();
    void onModbusWriteClicked();
    void onModbusConnectionStateChanged(bool connected, const QString &msg);
    void onModbusReadDataReady(const QVector<quint16> &data);
    void onModbusWriteDataReady(int address, quint16 value);
    void onModbusError(const QString &errorMsg);


private:
    SerialPortManager *m_serialMgr;
    QThread           *m_serialThread;   // 工作线程
    ProtocolParser    *m_parser;
    QCheckBox         *m_parseCheckBox;   // “协议解析”开关
    bool m_isPortOpen = false;        // ★ 新增：UI层维护的串口状态（避免跨线程读取 m_serialMgr）
    bool m_waitingForModbusConnect = false; // ★ 新增：标记是否在等待串口关闭以连接 Modbus

    // 界面控件
    QComboBox   *m_portComboBox;
    QComboBox   *m_baudComboBox;
    QPushButton *m_openCloseButton;
    QTextEdit   *m_recvTextEdit;
    QLineEdit   *m_sendLineEdit;
    QPushButton *m_sendButton;
    QCheckBox   *m_hexSendCheckBox;
    QCheckBox   *m_hexRecvCheckBox;
    QPushButton *m_clearRecvButton;
    QLabel      *m_countLabel;

    QCheckBox   *m_buildFrameCheckBox; // 协议组帧开关
    QCheckBox   *m_autoReplyCheckBox;  // 自动应答开关

    QCustomPlot *m_customPlot;  //波形图控件
    QTimer *m_dataTimer;        // 模拟数据定时器

    QTimer *m_modbusPollTimer; //Modbus 轮询定时器
    QCheckBox *m_autoPollCheckBox; // 自动轮询勾选框

    ModbusManager *m_modbusMgr;

    // ★ 新增：Modbus 调试区 UI 控件
    QSpinBox    *m_slaveIdSpin;      // 从站地址
    QComboBox   *m_funcCodeCombo;    // 功能码
    QSpinBox    *m_startAddrSpin;    // 起始地址
    QSpinBox    *m_regCountSpin;     // 寄存器数量 (读)
    QSpinBox    *m_regValueSpin;     // 寄存器值 (写)
    QPushButton *m_modbusConnectBtn; // 连接/断开按钮
    QPushButton *m_modbusReadBtn;    // 读按钮
    QPushButton *m_modbusWriteBtn;   // 写按钮
    QLineEdit   *m_modbusResultEdit; // 结果显示

    // 用于记录波形数据
    QVector<double> m_timeData; // X轴：时间
    QVector<double> m_valueData;// Y轴：数值
    double m_lastTime;          // 记录上一个时间点

    int m_recvCount = 0;
    int m_sendCount = 0;

    void initUI();
    void detectSerialPorts();
    void updateCountLabel();
    void addDataToPlot(double value);
    QByteArray hexStringToByteArray(const QString &hexStr);
};

#endif // MAINWINDOW_H