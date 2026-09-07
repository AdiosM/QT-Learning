#include "packetparser.h"

namespace lantransfer {

void PacketParser::append(const QByteArray& data)
{
    if (!data.isEmpty())
        m_buffer.append(data);
}

bool PacketParser::takePacket(Packet* out)
{
    if (!out || m_error != Error::None)
        return false;

    for (;;) {
        if (m_buffer.size() < HEADER_LENGTH)
            return false; // 等待更多字节（半包 Header）

        PacketHeader header;
        if (!PacketHeader::decode(m_buffer.constData(), m_buffer.size(), &header))
            return false; // 理论上不可达（长度已校验）

        // 逐字段校验，异常帧直接置错并丢弃缓冲（设计文档 §9.1）
        if (header.magic != PROTOCOL_MAGIC) {
            m_error = Error::BadMagic;
            m_buffer.clear();
            return false;
        }
        if (header.headerLength != HEADER_LENGTH) {
            m_error = Error::BadHeaderLength;
            m_buffer.clear();
            return false;
        }
        if (header.payloadLength > MAX_FRAME_PAYLOAD) {
            m_error = Error::PayloadTooLarge;
            m_buffer.clear();
            return false;
        }
        const quint8 type = header.type;
        if (type < static_cast<quint8>(MessageType::Hello) || type > messageTypeMaxValue()) {
            m_error = Error::UnknownType;
            m_buffer.clear();
            return false;
        }

        const qint64 total = HEADER_LENGTH + qint64(header.payloadLength);
        if (m_buffer.size() < total)
            return false; // 等待更多字节（半包 Payload）

        out->header = header;
        out->payload = m_buffer.mid(HEADER_LENGTH, qint64(header.payloadLength));
        m_buffer.remove(0, total);
        return true;
    }
}

void PacketParser::reset()
{
    m_buffer.clear();
    m_error = Error::None;
}

} // namespace lantransfer
