#pragma once

#include <QAbstractListModel>
#include <QHostAddress>
#include <QObject>
#include <QSettings>
#include <QVector>

#include "device.h"

// ============================================================================
// DeviceManager：设备表（设计文档 §5.2/§16）
//   - 本机设备身份（持久化 UUID、设备名、局域网 IP 枚举）
//   - 远端设备增改与在线/离线维护（8s 无更新离线）
//   - DeviceModel（QAbstractListModel）供 Widgets 与 QML 共用
// ============================================================================
namespace lantransfer {

class DeviceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        DeviceIdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        IpRole,
        TcpPortRole,
        AppVersionRole,
        OnlineRole,
    };
    Q_ENUM(Role)

    explicit DeviceModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const Device* deviceAt(int row) const;
    int indexOfDevice(const QString& deviceId) const;

    void setDevices(const QVector<Device>& devices);

private:
    QVector<Device> m_devices;
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject* parent = nullptr);

    // ---- 本机身份 ----
    QString localDeviceId() const { return m_deviceId; }
    // 覆写设备 ID（命令行 --device-id / 平台层自定义身份）
    void setLocalDeviceId(const QString& deviceId) { m_deviceId = deviceId; }
    QString localDeviceName() const { return m_deviceName; }
    void setLocalDeviceName(const QString& name);   // Android 端可用 JNI 覆盖主机名
    QString localDeviceType() const { return m_deviceType; }
    QString appVersion() const { return m_appVersion; }

    // ---- 本机局域网 IPv4 地址枚举（Windows 专项 §15）----
    static QList<QHostAddress> localIPv4Addresses();

    // ---- 远端设备表 ----
    DeviceModel* model() { return &m_model; }

    // 由 UDP 发现报文更新（ip 为报文 source address，优先于自报 IP）
    void upsertDevice(const Device& device);

    // 手动 IP 连接时添加/更新设备（视为在线）
    void upsertManualDevice(const QString& ip, quint16 tcpPort);

    Device* deviceById(const QString& deviceId);
    Device* deviceByIp(const QString& ip);
    bool hasDevice(const QString& deviceId) const;

    // 超过 DEVICE_OFFLINE_TIMEOUT_MS 未更新的设备标记离线；返回离线设备 id 列表
    QStringList refreshOfflineStatus(qint64 nowMs);

    QVector<Device> allDevices() const;

signals:
    void devicesChanged();

private:
    QString m_deviceId;
    QString m_deviceName;
    QString m_deviceType;
    QString m_appVersion;
    DeviceModel m_model;
    QVector<Device> m_devices;
};

} // namespace lantransfer
