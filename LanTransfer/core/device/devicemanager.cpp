#include "devicemanager.h"

#include <QDateTime>
#include <QNetworkInterface>
#include <QSysInfo>
#include <QUuid>

#include "protocol/packet.h"

namespace lantransfer {

// ---------------------------------------------------------------------------
// DeviceModel
// ---------------------------------------------------------------------------
DeviceModel::DeviceModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int DeviceModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_devices.size();
}

QVariant DeviceModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size())
        return {};
    const Device& d = m_devices.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:      return d.name;
    case DeviceIdRole:  return d.deviceId;
    case TypeRole:      return d.type;
    case IpRole:        return d.ip;
    case TcpPortRole:   return int(d.tcpPort);
    case AppVersionRole:return d.appVersion;
    case OnlineRole:    return d.online;
    }
    return {};
}

QHash<int, QByteArray> DeviceModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {DeviceIdRole, "deviceId"},
        {TypeRole, "type"},
        {IpRole, "ip"},
        {TcpPortRole, "tcpPort"},
        {AppVersionRole, "appVersion"},
        {OnlineRole, "online"},
    };
}

const Device* DeviceModel::deviceAt(int row) const
{
    if (row < 0 || row >= m_devices.size())
        return nullptr;
    return &m_devices.at(row);
}

int DeviceModel::indexOfDevice(const QString& deviceId) const
{
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices.at(i).deviceId == deviceId)
            return i;
    }
    return -1;
}

void DeviceModel::setDevices(const QVector<Device>& devices)
{
    beginResetModel();
    m_devices = devices;
    endResetModel();
}

// ---------------------------------------------------------------------------
// DeviceManager
// ---------------------------------------------------------------------------
DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent)
{
    // 首次运行生成并持久化 UUID，不随重启变化（设计文档 §5.2）
    QSettings settings;
    m_deviceId = settings.value(QStringLiteral("device/deviceId")).toString();
    if (m_deviceId.isEmpty()) {
        m_deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue(QStringLiteral("device/deviceId"), m_deviceId);
    }

    m_deviceName = QSysInfo::machineHostName();
    m_deviceType = QStringLiteral("windows");
#ifdef Q_OS_ANDROID
    m_deviceType = QStringLiteral("android");
#endif
    m_appVersion = QStringLiteral(LANTRANSFER_APP_VERSION);
}

void DeviceManager::setLocalDeviceName(const QString& name)
{
    if (!name.isEmpty() && name != m_deviceName) {
        m_deviceName = name;
        QSettings settings;
        settings.setValue(QStringLiteral("device/deviceName"), name);
    }
}

QList<QHostAddress> DeviceManager::localIPv4Addresses()
{
    QList<QHostAddress> result;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if ((iface.flags() & QNetworkInterface::IsUp) == 0
            || (iface.flags() & QNetworkInterface::IsRunning) == 0)
            continue;
        if (iface.flags() & QNetworkInterface::IsLoopBack)
            continue;
        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& e : entries) {
            const QHostAddress addr = e.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol)
                result.append(addr);
        }
    }
    return result;
}

void DeviceManager::upsertDevice(const Device& device)
{
    if (device.deviceId == m_deviceId)
        return; // 忽略自己

    for (Device& existing : m_devices) {
        if (existing.deviceId == device.deviceId) {
            const bool changed = existing.ip != device.ip || existing.name != device.name
                || existing.tcpPort != device.tcpPort || !existing.online;
            existing = device;
            if (changed)
                emit devicesChanged();
            return;
        }
    }
    m_devices.append(device);
    emit devicesChanged();
}

void DeviceManager::upsertManualDevice(const QString& ip, quint16 tcpPort)
{
    Device d;
    d.deviceId = QStringLiteral("manual:") + ip;
    d.name = ip;
    d.type = QStringLiteral("unknown");
    d.ip = ip;
    d.tcpPort = tcpPort;
    d.online = true;
    d.lastSeenMs = QDateTime::currentMSecsSinceEpoch();
    upsertDevice(d);
}

Device* DeviceManager::deviceById(const QString& deviceId)
{
    for (Device& d : m_devices) {
        if (d.deviceId == deviceId)
            return &d;
    }
    return nullptr;
}

Device* DeviceManager::deviceByIp(const QString& ip)
{
    for (Device& d : m_devices) {
        if (d.ip == ip)
            return &d;
    }
    return nullptr;
}

bool DeviceManager::hasDevice(const QString& deviceId) const
{
    for (const Device& d : m_devices) {
        if (d.deviceId == deviceId)
            return true;
    }
    return false;
}

QStringList DeviceManager::refreshOfflineStatus(qint64 nowMs)
{
    QStringList offlineIds;
    for (Device& d : m_devices) {
        if (d.online && nowMs - d.lastSeenMs > DEVICE_OFFLINE_TIMEOUT_MS) {
            d.online = false;
            offlineIds.append(d.deviceId);
        }
    }
    if (!offlineIds.isEmpty())
        emit devicesChanged();
    return offlineIds;
}

QVector<Device> DeviceManager::allDevices() const
{
    return m_devices;
}

} // namespace lantransfer
