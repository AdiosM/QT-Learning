#include "packet.h"

#include <cstring>

namespace lantransfer {

QByteArray PacketHeader::encode() const
{
    QByteArray b;
    b.resize(HEADER_LENGTH);
    char* p = b.data();
    qToBigEndian(magic, p);
    p += 4;
    *p++ = static_cast<char>(version);
    *p++ = static_cast<char>(type);
    qToBigEndian(flags, p);
    p += 2;
    qToBigEndian(headerLength, p);
    p += 2;
    qToBigEndian(payloadLength, p);
    p += 8;
    qToBigEndian(messageId, p);
    p += 8;
    const QByteArray tid = transferId.toRfc4122();
    std::memcpy(p, tid.constData(), 16);
    return b;
}

bool PacketHeader::decode(const char* data, qsizetype len, PacketHeader* out)
{
    if (!out || len < HEADER_LENGTH)
        return false;
    const uchar* p = reinterpret_cast<const uchar*>(data);
    out->magic = qFromBigEndian<quint32>(p);
    p += 4;
    out->version = *p++;
    out->type = *p++;
    out->flags = qFromBigEndian<quint16>(p);
    p += 2;
    out->headerLength = qFromBigEndian<quint16>(p);
    p += 2;
    out->payloadLength = qFromBigEndian<quint64>(p);
    p += 8;
    out->messageId = qFromBigEndian<quint64>(p);
    p += 8;
    out->transferId = QUuid::fromRfc4122(QByteArray(reinterpret_cast<const char*>(p), 16));
    return true;
}

} // namespace lantransfer
