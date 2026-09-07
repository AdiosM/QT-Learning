#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

// ============================================================================
// Device：局域网内一台设备的描述（设计文档 §5.2）
// 设备 IP 以 UDP 报文 source address 为准；TCP 端口由发现报文携带。
// ============================================================================
namespace lantransfer {

struct Device {
    QString deviceId;      // 首次运行生成并持久化的 UUID（不随重启变化）
    QString name;          // 设备名（可含中文）
    QString type;          // "windows" / "android"
    QString ip;            // 局域网 IPv4
    quint16 tcpPort = 0;   // 对端 TCP 监听端口（由发现报文携带）
    QString appVersion;
    QStringList capabilities;
    bool online = false;
    qint64 lastSeenMs = 0; // 最近一次收到发现报文的时刻
};

} // namespace lantransfer
