#include "serialportmanager.h"
#include <QSerialPortInfo>   // 只有枚举用到，放 .cpp 里

SerialPortManager::SerialPortManager(QObject *parent): QObject(parent), m_serial(new QSerialPort(this))   // 串口对象作为子对象，自动释放
{
    // 数据到达 → 读出来 → 用信号抛给上层
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialPortManager::onReadyRead);

    // 运行时错误（打开失败、设备被拔等）
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialPortManager::onErrorOccurred);
}

SerialPortManager::~SerialPortManager()
{
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

// ========== 枚举系统串口 ==========
QList<PortInfo> SerialPortManager::enumeratePorts()
{//只是问操作系统 "现在有哪些串口插着？
    QList<PortInfo> result;
    const auto infos = QSerialPortInfo::availablePorts();

    //遍历每个串口
    for (const QSerialPortInfo &info : infos) {
        result.append({info.portName(), info.description()});
    }
    return result;//返回系统当前所有可用的串口
}

bool SerialPortManager::isOpen() const
{
    return m_serial->isOpen();//m_serial是QSerialPort对象，这里的isOpen()是QSerialPort的成员函数
}

// ========== 打开串口 ==========
bool SerialPortManager::openPort(const SerialConfig &config)
{
    if (m_serial->isOpen()) {
        return true;   // 已经开着，直接算成功
    }

//m_serial是这个类的成员，与SerialPortManager对象的生命周期一样
    //串口没开，就先设置好物理参数
    m_serial->setPortName(config.portName);
    m_serial->setBaudRate(config.baudRate);
    m_serial->setDataBits(config.dataBits);
    m_serial->setParity(config.parity);
    m_serial->setStopBits(config.stopBits);
    m_serial->setFlowControl(config.flowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        // 打开失败：把底层错误原样上报
        emit portOpened(false, config.portName, m_serial->errorString());
        return false;
    }

    emit portOpened(true, config.portName, QString());//串口打开成功，发送信号
    return true;
}

// ========== 关闭串口 ==========
void SerialPortManager::closePort()
{
    if (!m_serial->isOpen()) {
        return;  //防御性编程
    }
    m_serial->close();
    emit portClosed();
}

// ========== 发送数据 ==========
qint64 SerialPortManager::sendData(const QByteArray &data)
{
    if (!m_serial->isOpen()) {
        return -1;
    }
    return m_serial->write(data);
}

// ========== 收到数据 ==========
void SerialPortManager::onReadyRead()
{
    // 通信层不做任何格式处理！原样抛给上层，
    // 显示成 HEX 还是 ASCII 是 UI 层的事
    emit dataReceived(m_serial->readAll());
}

// ========== 运行时错误 ==========
void SerialPortManager::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    const QString msg = m_serial->errorString();

    // ★ 设备被强行拔出：必须主动关闭，否则串口对象进入"僵尸状态"
    if (error == QSerialPort::ResourceError) {
        emit serialError(msg);
        closePort();          // 触发 portClosed，让 UI 自动复位
        return;
    }

    emit serialError(msg);
}