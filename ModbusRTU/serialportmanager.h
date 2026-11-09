#ifndef SERIALPORTMANAGER_H
#define SERIALPORTMANAGER_H

#include <QObject>
#include <QSerialPort>

//它只管物理层参数（波特率、数据位、校验、停止位、流控），不做任何格式 / 协议处理—— 收什么字节就原样上报
// 串口配置参数（物理层参数）：把一堆 set 调用打包成一个结构体，
// 以后UI层只需要填这个结构体，不用知道 QSerialPort 的存在
struct SerialConfig
{
    QString    portName;//端口名，如COM1
    qint32     baudRate   = 115200; //波特率，默认值
    QSerialPort::DataBits     dataBits   = QSerialPort::Data8;//数据位8
    QSerialPort::Parity       parity     = QSerialPort::NoParity;//校验位，无
    QSerialPort::StopBits     stopBits   = QSerialPort::OneStop;//停止位，1
    QSerialPort::FlowControl  flowControl= QSerialPort::NoFlowControl;//流控，1
};

// 串口信息（给 UI 下拉框用），避免 UI 层直接依赖 QSerialPortInfo
struct PortInfo
{
    QString portName;
    QString description;
};

class SerialPortManager : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortManager(QObject *parent = nullptr);
    ~SerialPortManager() override;

    // ---- 命令接口（UI 主动调用）----
    bool   openPort(const SerialConfig &config);   // 打开串口
    void   closePort();                             // 关闭串口
    qint64 sendData(const QByteArray &data);        // 发送数据，返回写入字节数
    bool   isOpen() const;                          // 查询状态

    // ---- 设备枚举（静态函数，不依赖实例）----
    static QList<PortInfo> enumeratePorts();//PortInfo列表

signals:
    // ---- 事件接口（异步反馈给 UI）----
    void portOpened(bool ok, const QString &portName, const QString &errMsg);
    void portClosed();
    void dataReceived(const QByteArray &data);      // 收到数据
    void serialError(const QString &errMsg);        // 运行时错误（如设备被拔）

private slots:
    void onReadyRead();// QSerialPort 数据到达
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serial = nullptr; //真正的串口对象
};

#endif // SERIALPORTMANAGER_H