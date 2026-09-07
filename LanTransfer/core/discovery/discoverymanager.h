#pragma once

#include <QHostAddress>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>

#include "device/devicemanager.h"
#include "protocol/packet.h"

// ============================================================================
// DiscoveryManager：UDP 设备发现（设计文档 §5.2）
//   UDP 50001：广播 DISCOVERY_REQUEST（2s 周期）-> 收到 REQUEST 回 RESPONSE
//   -> 收到 RESPONSE 更新设备表（IP 以 source address 为准）
//   手动 IP 探测作为广播受限时的降级入口。
//   报文格式：4B magic "LNFT" + 1B version + 1B msgType + UTF-8 JSON
// ============================================================================
namespace lantransfer {

class DiscoveryManager : public QObject
{
    Q_OBJECT
public:
    explicit DiscoveryManager(DeviceManager* devices, QObject* parent = nullptr);

    // 绑定 UDP 端口并启动 2s 广播 + 离线巡检；失败返回 false
    bool start(quint16 udpPort = DEFAULT_UDP_PORT);
    void stop();
    bool isRunning() const { return m_socket.state() == QAbstractSocket::BoundState; }

    // 设置本端 TCP 监听端口（随发现报文携带，设计文档 §5.1）
    void setTcpPort(quint16 port) { m_tcpPort = port; }
    quint16 tcpPort() const { return m_tcpPort; }

    quint16 udpPort() const { return m_socket.localPort(); }
    QString errorString() const { return m_error; }

    // 手动 IP 探测（广播受限降级入口）：向指定 IP 的 UDP 端口发送 REQUEST
    void sendProbe(const QString& ip, quint16 udpPort);

signals:
    void logMessage(const QString& message);

private slots:
    void onReadyRead();
    void onTimer();

private:
    void sendRequest(const QHostAddress& dest);
    void handleDatagram(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);

    DeviceManager* m_devices = nullptr;
    QUdpSocket m_socket;
    QTimer m_timer;
    quint16 m_tcpPort = DEFAULT_TCP_PORT;
    QString m_error;
};

} // namespace lantransfer
