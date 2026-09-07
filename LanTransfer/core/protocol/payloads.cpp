#include "payloads.h"

#include <QJsonArray>

namespace lantransfer {

QString errorMessage(ErrorCode code)
{
    switch (code) {
    case ErrorCode::Ok:            return QStringLiteral("成功");
    case ErrorCode::FileTooLarge:  return QStringLiteral("文件超过 500 MB 限制");
    case ErrorCode::DiskFull:      return QStringLiteral("磁盘空间不足");
    case ErrorCode::Incomplete:    return QStringLiteral("文件不完整，接收字节数与声明大小不一致");
    case ErrorCode::ProtocolError: return QStringLiteral("协议错误");
    case ErrorCode::Timeout:       return QStringLiteral("连接或操作超时");
    case ErrorCode::UserRejected:  return QStringLiteral("对方拒绝传输");
    case ErrorCode::Cancelled:     return QStringLiteral("传输已取消");
    case ErrorCode::VerifyFailed:  return QStringLiteral("SHA-256 校验失败，文件可能已损坏");
    case ErrorCode::InvalidName:   return QStringLiteral("文件名非法");
    case ErrorCode::Busy:          return QStringLiteral("对方设备正忙");
    case ErrorCode::NotFound:      return QStringLiteral("文件不存在或不可读");
    case ErrorCode::PinMismatch:   return QStringLiteral("配对 PIN 不匹配");
    case ErrorCode::Disconnected:  return QStringLiteral("连接已断开");
    case ErrorCode::InternalError: return QStringLiteral("内部错误");
    }
    return QStringLiteral("未知错误(%1)").arg(static_cast<qint32>(code));
}

static QJsonArray stringListToJson(const QStringList& list)
{
    QJsonArray arr;
    for (const QString& s : list)
        arr.append(s);
    return arr;
}

static QStringList jsonToStringList(const QJsonValue& v)
{
    QStringList list;
    if (!v.isArray())
        return list;
    const QJsonArray arr = v.toArray();
    for (const QJsonValue& item : arr)
        if (item.isString())
            list.append(item.toString());
    return list;
}

QJsonObject HelloPayload::toJson() const
{
    return QJsonObject{
        {QStringLiteral("deviceId"), deviceId},
        {QStringLiteral("deviceName"), deviceName},
        {QStringLiteral("deviceType"), deviceType},
        {QStringLiteral("tcpPort"), qint64(tcpPort)},
        {QStringLiteral("appVersion"), appVersion},
        {QStringLiteral("capabilities"), stringListToJson(capabilities)},
    };
}

HelloPayload HelloPayload::fromJson(const QJsonObject& obj)
{
    HelloPayload p;
    p.deviceId = obj.value(QStringLiteral("deviceId")).toString();
    p.deviceName = obj.value(QStringLiteral("deviceName")).toString();
    p.deviceType = obj.value(QStringLiteral("deviceType")).toString();
    p.tcpPort = quint16(obj.value(QStringLiteral("tcpPort")).toInt());
    p.appVersion = obj.value(QStringLiteral("appVersion")).toString();
    p.capabilities = jsonToStringList(obj.value(QStringLiteral("capabilities")));
    return p;
}

QJsonObject PairRequestPayload::toJson() const
{
    return QJsonObject{
        {QStringLiteral("deviceName"), deviceName},
        {QStringLiteral("appVersion"), appVersion},
        {QStringLiteral("fileName"), fileName},
        {QStringLiteral("fileSize"), qint64(fileSize)},
    };
}

PairRequestPayload PairRequestPayload::fromJson(const QJsonObject& obj)
{
    PairRequestPayload p;
    p.deviceName = obj.value(QStringLiteral("deviceName")).toString();
    p.appVersion = obj.value(QStringLiteral("appVersion")).toString();
    p.fileName = obj.value(QStringLiteral("fileName")).toString();
    p.fileSize = quint64(obj.value(QStringLiteral("fileSize")).toDouble());
    return p;
}

QJsonObject PairResponsePayload::toJson() const
{
    return QJsonObject{
        {QStringLiteral("accepted"), accepted},
        {QStringLiteral("pin"), pin},
    };
}

PairResponsePayload PairResponsePayload::fromJson(const QJsonObject& obj)
{
    PairResponsePayload p;
    p.accepted = obj.value(QStringLiteral("accepted")).toBool();
    p.pin = obj.value(QStringLiteral("pin")).toString();
    return p;
}

QJsonObject FileInfoPayload::toJson() const
{
    return QJsonObject{
        {QStringLiteral("fileId"), fileId},
        {QStringLiteral("fileName"), fileName},
        {QStringLiteral("fileSize"), qint64(fileSize)},
        {QStringLiteral("mimeType"), mimeType},
        {QStringLiteral("sha256"), sha256},
        {QStringLiteral("relativePath"), relativePath},
    };
}

FileInfoPayload FileInfoPayload::fromJson(const QJsonObject& obj)
{
    FileInfoPayload p;
    p.fileId = obj.value(QStringLiteral("fileId")).toString();
    p.fileName = obj.value(QStringLiteral("fileName")).toString();
    p.fileSize = quint64(obj.value(QStringLiteral("fileSize")).toDouble());
    p.mimeType = obj.value(QStringLiteral("mimeType")).toString();
    p.sha256 = obj.value(QStringLiteral("sha256")).toString();
    p.relativePath = obj.value(QStringLiteral("relativePath")).toString();
    return p;
}

QJsonObject FileAcceptPayload::toJson() const
{
    return QJsonObject{{QStringLiteral("accepted"), accepted}};
}

FileAcceptPayload FileAcceptPayload::fromJson(const QJsonObject& obj)
{
    FileAcceptPayload p;
    p.accepted = obj.value(QStringLiteral("accepted")).toBool();
    return p;
}

QJsonObject FileEndPayload::toJson() const
{
    return QJsonObject{{QStringLiteral("sha256"), sha256}};
}

FileEndPayload FileEndPayload::fromJson(const QJsonObject& obj)
{
    FileEndPayload p;
    p.sha256 = obj.value(QStringLiteral("sha256")).toString();
    return p;
}

QJsonObject FileVerifyPayload::toJson() const
{
    return QJsonObject{
        {QStringLiteral("success"), success},
        {QStringLiteral("sha256"), sha256},
    };
}

FileVerifyPayload FileVerifyPayload::fromJson(const QJsonObject& obj)
{
    FileVerifyPayload p;
    p.success = obj.value(QStringLiteral("success")).toBool();
    p.sha256 = obj.value(QStringLiteral("sha256")).toString();
    return p;
}

QJsonObject CancelPayload::toJson() const
{
    return QJsonObject{{QStringLiteral("reason"), reason}};
}

CancelPayload CancelPayload::fromJson(const QJsonObject& obj)
{
    CancelPayload p;
    p.reason = obj.value(QStringLiteral("reason")).toString();
    return p;
}

QJsonObject ErrorPayload::toJson() const
{
    return QJsonObject{
        {QStringLiteral("code"), code},
        {QStringLiteral("message"), message},
    };
}

ErrorPayload ErrorPayload::fromJson(const QJsonObject& obj)
{
    ErrorPayload p;
    p.code = obj.value(QStringLiteral("code")).toInt();
    p.message = obj.value(QStringLiteral("message")).toString();
    return p;
}

} // namespace lantransfer
