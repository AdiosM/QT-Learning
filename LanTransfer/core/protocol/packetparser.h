#pragma once

#include "packet.h"

#include <QByteArray>

// ============================================================================
// PacketParser：TCP 字节流 -> 完整帧 的增量解析状态机（设计文档 §9.1）
//   WAIT_HEADER -> READ_PAYLOAD -> DISPATCH -> WAIT_HEADER
// 要点：
//   - 不假设一次 readyRead 就是一个完整消息（处理拆包/粘包）
//   - PayloadLength 先做上限检查再取 Payload
//   - 异常帧置错误并清空缓冲区，由会话层关闭连接，避免协议失步
// ============================================================================
namespace lantransfer {

class PacketParser
{
public:
    enum class Error {
        None = 0,
        BadMagic,          // Magic 不匹配
        BadHeaderLength,   // HeaderLength != 42
        PayloadTooLarge,   // PayloadLength > 4 MiB
        UnknownType,       // Type 不在消息类型枚举内
    };

    struct Packet {
        PacketHeader header;
        QByteArray payload;
    };

    // 追加新到达的字节（典型用法：每次 readyRead 调用一次）
    void append(const QByteArray& data);

    // 提取一个完整帧；无完整帧返回 false。发生协议错误时置 lastError 并丢弃缓冲。
    // 调用方应循环调用直到返回 false。
    bool takePacket(Packet* out);

    Error lastError() const { return m_error; }
    qint64 bufferedBytes() const { return m_buffer.size(); }

    // 出错后恢复可用状态（新会话复用解析器时调用）
    void reset();

private:
    QByteArray m_buffer;
    Error m_error = Error::None;
};

} // namespace lantransfer
