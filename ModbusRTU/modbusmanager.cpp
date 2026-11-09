#include "modbusmanager.h"
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QDebug>
#include <QSerialPort>
#include <QVariant>
ModbusManager::ModbusManager(QObject *parent)
    : QObject(parent),
      m_modbusDevice(new QModbusRtuSerialClient(this))
{
    // 监听连接状态变化
    connect(m_modbusDevice, &QModbusClient::stateChanged,
            this, &ModbusManager::onStateChanged);
    // 监听错误
    connect(m_modbusDevice, &QModbusClient::errorOccurred,
            this, &ModbusManager::onErrorOccurred);
}

ModbusManager::~ModbusManager()
{
    if (m_modbusDevice->state() != QModbusDevice::UnconnectedState) {
        m_modbusDevice->disconnectDevice();
    }
}

bool ModbusManager::isConnected() const
{
    return m_modbusDevice->state() == QModbusDevice::ConnectedState;
}

void ModbusManager::connectDevice(const QString &portName, int baudRate)
{
    if (m_modbusDevice->state() != QModbusDevice::UnconnectedState) {
        return;
    }

    // 设置串口物理参数
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QVariant(portName));//端口名称
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QVariant(baudRate));//波特率
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QVariant(QSerialPort::Data8));//8数据位
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, QVariant(QSerialPort::NoParity));//无校验
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QVariant(QSerialPort::OneStop));//1停止位

    // 设置超时和重试，
    m_modbusDevice->setTimeout(1000); // 每次请求 1 秒没回应就算超时
    m_modbusDevice->setNumberOfRetries(3); // 失败自动重试3次

    if (!m_modbusDevice->connectDevice()) {
        emit errorOccurred("连接失败: " + m_modbusDevice->errorString());
    }
}

void ModbusManager::disconnectDevice()
{
    m_modbusDevice->disconnectDevice();
    //调用的是QModbusRtuSerialClient类中的方法disconnectDevice，功能是断开连接
}

void ModbusManager::onStateChanged(int state)
{
    if (state == QModbusDevice::ConnectedState) {
        emit connectionStateChanged(true, "Modbus 连接成功");
    } else if (state == QModbusDevice::UnconnectedState) {
        emit connectionStateChanged(false, "Modbus 已断开");
    }
}

void ModbusManager::onErrorOccurred(QModbusDevice::Error error)
{
    if (error != QModbusDevice::NoError) {
        emit errorOccurred("Modbus 错误: " + m_modbusDevice->errorString());
    }
}

// ========== 读保持寄存器 (功能码 03) ==========
void ModbusManager::readHoldingRegisters(int startAddress, int count)
{
    if (!isConnected()) return;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddress, count);
    //第一个参数是寄存器类型，第二个是起始地址（从哪个寄存器开始操作），第三个是数量（连续操作多少个寄存器）

    // 异步读取，发送请求，拿回一个"回执单" reply
    //sendReadRequest 是 QModbusClient（Modbus 客户端 / 主站）的 "发读请求" 方法
    //它把 "要读哪些寄存器" 打包成 Modbus 报文，发给指定的从站，并立刻返回一个回复凭证 QModbusReply*（不等待结果，异步）
    if (auto *reply = m_modbusDevice->sendReadRequest(readUnit, 1)) { // 1 是从站地址
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusManager::onReadReplyFinished);
            //正常情况：从站还没回，挂上信号等通知
        } else {
            //特殊情况：立即完成（如广播），直接销毁防泄漏
            delete reply; // 广播请求会立即返回 finished
        }
    } else {
        emit errorOccurred("读取请求发送失败: " + m_modbusDevice->errorString());
    }
}

void ModbusManager::onReadReplyFinished()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());//sender() 返回 "谁发出的这个信号"
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();//result() 返回 QModbusDataUnit—— 里面包含起始地址、数量、以及每个寄存器的值
        QVector<quint16> data = unit.values();
        emit readDataReady(data);
    } else {
        emit errorOccurred("读取响应错误: " + reply->errorString());
    }
    reply->deleteLater();//deleteLater() 是 "等回到事件循环空闲时再删"
}

// ========== 写单个寄存器 (功能码 06) ==========
void ModbusManager::writeSingleRegister(int address, quint16 value)
{
    if (!isConnected()) return;

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    writeUnit.setValue(0, value);

    if (auto *reply = m_modbusDevice->sendWriteRequest(writeUnit, 1)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusManager::onWriteReplyFinished);
        } else {
            delete reply;
        }
    } else {
        emit errorOccurred("写入请求发送失败: " + m_modbusDevice->errorString());
    }
}

void ModbusManager::onWriteReplyFinished()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        emit writeDataReady(unit.startAddress(), unit.value(0));
    } else {
        emit errorOccurred("写入响应错误: " + reply->errorString());
    }
    reply->deleteLater();
}