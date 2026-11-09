#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <QObject>
#include <QModbusRtuSerialClient>
#include <QVector>

//将 Qt 官方复杂的 Modbus API 封装起来，为 UI 层提供简单、安全、异步的 Modbus RTU 读写接口。
class ModbusManager : public QObject
{
    Q_OBJECT
public:
    explicit ModbusManager(QObject *parent = nullptr);
    ~ModbusManager() override;

    // 连接/断开设备
    void connectDevice(const QString &portName, int baudRate);//一键配置并连接串口
    void disconnectDevice();//断开连接
    bool isConnected() const;

    // 读写操作（异步）
    void readHoldingRegisters(int startAddress, int count);//读保持寄存器
    void writeSingleRegister(int address, quint16 value);//写单个寄存器
signals:
    void connectionStateChanged(bool connected, const QString &msg);//通知 UI 层更新按钮文字（“连接” ↔ “断开”）
    void readDataReady(const QVector<quint16> &data);
    void writeDataReady(int address, quint16 value);
    void errorOccurred(const QString &errorMsg);

private slots:
    void onStateChanged(int state);
    void onErrorOccurred(QModbusDevice::Error error);
    void onReadReplyFinished();
    void onWriteReplyFinished();

private:
    //QModbusRtuSerialClient 是Qt官方提供的、用于实现 Modbus RTU 协议"主站（客户端）"的类。
    //QModbusRtuSerialClient提供多个API接口，主要功能为:设置串口号，设置波特率，设置数据位/校验/停止位，建立连接，断开连接
    QModbusRtuSerialClient *m_modbusDevice;
};

#endif // MODBUSMANAGER_H
