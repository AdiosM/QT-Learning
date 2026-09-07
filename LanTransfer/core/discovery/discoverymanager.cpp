#include "discoverymanager.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include "protocol/packet.h"

namespace lantransfer {

// 发现报文类型
namespace {
constexpr quint8 DiscoveryRequest = 0x01;
constexpr quint8 DiscoveryResponse = 0x02;
}

DiscoveryManager::DiscoveryManager(DeviceManager* devices, QObject* parent)
    : QObject(parent)
    , m_devices(devices)
{
    m_timer.setInterval(DISCOVERY_INTERVAL_MS);

    connect(&m_timer, &QTimer::timeout, this, &DiscoveryManager::onTimer);
    connect(&m_socket, &QUdpSocket::readyRead, this, &DiscoveryManager::onReadyRead);
}

bool DiscoveryManager::start(quint16 udpPort)
{
    m_error.clear();
    if (m_socket.state() == QAbstractSocket::BoundState)
        return true;

    if (!m_socket.bind(QHostAddress::AnyIPv4, udpPort, QUdpSocket::ShareAddress
                       | QUdpSocket::ReuseAddressHint)) {
        m_error = QStringLiteral("UDP 端口 %1 绑定失败：%2").arg(udpPort).arg(m_socket.errorString());
        return false;
    }

    // 立即广播一次，之后按 2s 周期
    onTimer();
    m_timer.start();
    emit logMessage(QStringLiteral("设备发现已启动（UDP %1，TCP %2）").arg(m_socket.localPort()).arg(m_tcpPort));
    return true;
}

void DiscoveryManager::stop()
{
    m_timer.stop();
    if (m_socket.state() == QAbstractSocket::BoundState)
        m_socket.close();
}

void DiscoveryManager::onTimer()
{
    // 广播：255.255.255.255 及各活跃接口的广播地址（覆盖多网卡场景）
    sendRequest(QHostAddress::Broadcast);
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if ((iface.flags() & QNetworkInterface::IsUp) == 0)
            continue;
        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& e : entries) {
            if (e.ip().protocol() == QAbstractSocket::IPv4Protocol)
                sendRequest(e.broadcast());
        }
    }
    // 离线巡检：8s 无更新标记离线
    m_devices->refreshOfflineStatus(QDateTime::currentMSecsSinceEpoch());
}

void DiscoveryManager::sendRequest(const QHostAddress& dest)
{
    if (dest.isNull())
        return;
    const QJsonObject payload{
        {QStringLiteral("deviceId"), m_devices->localDeviceId()},
        {QStringLiteral("deviceName"), m_devices->localDeviceName()},
        {QStringLiteral("deviceType"), m_devices->localDeviceType()},
        {QStringLiteral("tcpPort"), qint64(m_tcpPort)},
        {QStringLiteral("appVersion"), m_devices->appVersion()},
        {QStringLiteral("capabilities"), QJsonArray{QStringLiteral("file-transfer")}},
    };
    QByteArray datagram = QByteArray::fromRawData("LNFT", 4);
    datagram.append(char(PROTOCOL_VERSION));
    datagram.append(char(DiscoveryRequest));
    datagram.append(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_socket.writeDatagram(datagram, dest, m_socket.localPort());
}

void DiscoveryManager::sendProbe(const QString& ip, quint16 udpPort)
{
    const QJsonObject payload{
        {QStringLiteral("deviceId"), m_devices->localDeviceId()},
        {QStringLiteral("deviceName"), m_devices->localDeviceName()},
        {QStringLiteral("deviceType"), m_devices->localDeviceType()},
        {QStringLiteral("tcpPort"), qint64(m_tcpPort)},
        {QStringLiteral("appVersion"), m_devices->appVersion()},
        {QStringLiteral("capabilities"), QJsonArray{QStringLiteral("file-transfer")}},
    };
    QByteArray datagram = QByteArray::fromRawData("LNFT", 4);
    datagram.append(char(PROTOCOL_VERSION));
    datagram.append(char(DiscoveryRequest));
    datagram.append(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_socket.writeDatagram(datagram, QHostAddress(ip), udpPort);
}

void DiscoveryManager::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram();
        handleDatagram(datagram.data(), datagram.senderAddress(), datagram.senderPort());
    }
}

void DiscoveryManager::handleDatagram(const QByteArray& data, const QHostAddress& sender,
                                      quint16 senderPort)
{
    // 校验 magic + version + 最小长度
    if (data.size() < 6 || !data.startsWith("LNFT"))
        return;
    if (quint8(data.at(4)) != PROTOCOL_VERSION)
        return;
    const quint8 msgType = quint8(data.at(5));
    if (msgType != DiscoveryRequest && msgType != DiscoveryResponse)
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(data.mid(6));
    if (doc.isNull() || !doc.isObject())
        return;
    const QJsonObject obj = doc.object();

    const QString deviceId = obj.value(QStringLiteral("deviceId")).toString();
    if (deviceId.isEmpty() || deviceId == m_devices->localDeviceId())
        return; // 忽略空/自己的报文

    const QHostAddress senderIp = sender;
    if (senderIp.isNull() || senderIp.protocol() != QAbstractSocket::IPv4Protocol)
        return;

    if (msgType == DiscoveryRequest) {
        // 请求方信息同样可信（设备身份最终由 TCP HELLO 确认，设计文档 §5.2）。
        // 双方都从收到的报文登记对方，发现才是对称的——避免"同端口多 socket 时
        // 单播只投递给其中一个绑定者"导致某一方永远学不到对方。
        Device requester;
        requester.deviceId = deviceId;
        requester.name = obj.value(QStringLiteral("deviceName")).toString();
        requester.type = obj.value(QStringLiteral("deviceType")).toString();
        requester.ip = senderIp.toString();
        requester.tcpPort = quint16(obj.value(QStringLiteral("tcpPort")).toInt());
        requester.appVersion = obj.value(QStringLiteral("appVersion")).toString();
        const QJsonArray reqCaps = obj.value(QStringLiteral("capabilities")).toArray();
        for (const QJsonValue& v : reqCaps)
            if (v.isString())
                requester.capabilities.append(v.toString());
        requester.online = true;
        requester.lastSeenMs = QDateTime::currentMSecsSinceEpoch();
        if (!m_devices->hasDevice(deviceId))
            emit logMessage(QStringLiteral("发现设备：%1（%2:%3）")
                                .arg(requester.name, requester.ip).arg(requester.tcpPort));
        m_devices->upsertDevice(requester);

        // 同时回复 RESPONSE（单播回 source address）
        const QJsonObject resp{
            {QStringLiteral("deviceId"), m_devices->localDeviceId()},
            {QStringLiteral("deviceName"), m_devices->localDeviceName()},
            {QStringLiteral("deviceType"), m_devices->localDeviceType()},
            {QStringLiteral("tcpPort"), qint64(m_tcpPort)},
            {QStringLiteral("appVersion"), m_devices->appVersion()},
            {QStringLiteral("capabilities"), QJsonArray{QStringLiteral("file-transfer")}},
        };
        QByteArray datagram = QByteArray::fromRawData("LNFT", 4);
        datagram.append(char(PROTOCOL_VERSION));
        datagram.append(char(DiscoveryResponse));
        datagram.append(QJsonDocument(resp).toJson(QJsonDocument::Compact));
        // 单播回复到请求方地址与端口（不可用本端 localPort 替代）
        m_socket.writeDatagram(datagram, senderIp, senderPort);
        return;
    }

    // RESPONSE：更新设备表，IP 以 source address 为准（设计文档 §5.2）
    Device device;
    device.deviceId = deviceId;
    device.name = obj.value(QStringLiteral("deviceName")).toString();
    device.type = obj.value(QStringLiteral("deviceType")).toString();
    device.ip = senderIp.toString();
    device.tcpPort = quint16(obj.value(QStringLiteral("tcpPort")).toInt());
    device.appVersion = obj.value(QStringLiteral("appVersion")).toString();
    const QJsonArray caps = obj.value(QStringLiteral("capabilities")).toArray();
    for (const QJsonValue& v : caps)
        if (v.isString())
            device.capabilities.append(v.toString());
    device.online = true;
    device.lastSeenMs = QDateTime::currentMSecsSinceEpoch();
    if (!m_devices->hasDevice(deviceId))
        emit logMessage(QStringLiteral("发现设备：%1（%2:%3）")
                            .arg(device.name, device.ip).arg(device.tcpPort));
    m_devices->upsertDevice(device);
}

} // namespace lantransfer
