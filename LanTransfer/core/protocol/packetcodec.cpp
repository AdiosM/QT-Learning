#include "packetcodec.h"

#include <QAtomicInteger>
#include <QJsonDocument>

namespace lantransfer {

QByteArray PacketCodec::encodeFrame(const PacketHeader& header, const QByteArray& payload)
{
    if (payload.size() > qint64(MAX_FRAME_PAYLOAD))
        return {}; // 调用方保证不越界；此处兜底
    PacketHeader h = header;
    h.payloadLength = quint64(payload.size());
    h.headerLength = HEADER_LENGTH;
    QByteArray frame = h.encode();
    frame.append(payload);
    return frame;
}

QByteArray PacketCodec::encodeControl(MessageType type, const QUuid& transferId,
                                      const QJsonObject& payload, quint64 messageId)
{
    PacketHeader h;
    h.type = static_cast<quint8>(type);
    h.transferId = transferId;
    h.messageId = messageId ? messageId : nextMessageId();
    const QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    if (json.size() > qint64(MAX_FRAME_PAYLOAD))
        return {};
    return encodeFrame(h, json);
}

QByteArray PacketCodec::encodeFileData(const QUuid& transferId, const QByteArray& chunk,
                                       quint64 messageId)
{
    PacketHeader h;
    h.type = static_cast<quint8>(MessageType::FileData);
    h.transferId = transferId;
    h.messageId = messageId ? messageId : nextMessageId();
    return encodeFrame(h, chunk);
}

std::optional<QJsonObject> PacketCodec::decodeControlJson(const QByteArray& payload)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return std::nullopt;
    return doc.object();
}

quint64 PacketCodec::nextMessageId()
{
    static QAtomicInteger<quint64> counter(1);
    return counter.fetchAndAddRelaxed(1);
}

} // namespace lantransfer
