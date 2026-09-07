#pragma once

#include <QByteArray>
#include <QUuid>
#include <QtGlobal>

// ============================================================================
// LanTransfer 应用层协议 V1.0（设计文档 §6）
// 帧头 42 字节固定长度，所有多字节字段均为 Big-Endian（网络字节序）。
// 控制消息 Payload 为 UTF-8 JSON；FILE_DATA 的 Payload 为原始文件字节。
// ============================================================================
namespace lantransfer {

// 帧头 Magic："LNFT"（0x4C 0x4E 0x46 0x54）
constexpr quint32 PROTOCOL_MAGIC = 0x4C4E4654u;
constexpr quint8  PROTOCOL_VERSION = 1;
constexpr quint16 HEADER_LENGTH = 42;                       // V1.0 帧头长度
constexpr quint64 MAX_FRAME_PAYLOAD = 4ull * 1024 * 1024;   // 4 MiB，控制/数据帧 Payload 上限
constexpr quint32 DEFAULT_CHUNK_SIZE = 1u * 1024 * 1024;    // FILE_DATA 默认分块 1 MiB

// 单文件硬限制：500 MB = 十进制 500,000,000 bytes（产品约束，发送端/接收端/协议校验三方执行）
constexpr quint64 MAX_FILE_SIZE = 500000000ull;

// 端口规划（设计文档 §5.1）：可被设置项覆盖；UDP 发现报文携带当前 TCP 端口
constexpr quint16 DEFAULT_TCP_PORT = 50000;
constexpr quint16 DEFAULT_UDP_PORT = 50001;

// 设备发现参数（设计文档 §5.2 / §19）
constexpr int DISCOVERY_INTERVAL_MS = 2000;   // 广播周期
constexpr int DEVICE_OFFLINE_TIMEOUT_MS = 8000; // 超过 8s 无更新判离线

// 发送流控（设计文档 §9.2）：bytesToWrite 高水位暂停读文件，低水位恢复
constexpr qint64 SEND_HIGH_WATERMARK = 8ll * 1024 * 1024;
constexpr qint64 SEND_LOW_WATERMARK = 2ll * 1024 * 1024;

// 消息类型（设计文档 §6.2）
enum class MessageType : quint8 {
    Hello = 0x01,         // 双向：协议版本、设备ID、能力协商
    PairRequest = 0x02,   // 发送方->接收方：请求允许本次传输
    PairResponse = 0x03,  // 接收方->发送方：ACCEPT/REJECT（含配对 PIN）
    FileInfo = 0x04,      // 发送方->接收方：文件元数据
    FileAccept = 0x05,    // 接收方->发送方：允许开始写入
    FileData = 0x06,      // 发送方->接收方：分块文件数据
    FileEnd = 0x07,       // 发送方->接收方：文件发送结束（携带发送端 SHA-256）
    FileVerify = 0x08,    // 接收方->发送方：SHA-256 结果及最终状态
    Cancel = 0x09,        // 任一方->任一方：主动取消
    Error = 0x0A,         // 任一方->任一方：错误码与可读说明
    SessionClose = 0x0B,  // 任一方->任一方：正常关闭会话
};

constexpr quint8 messageTypeMaxValue() { return static_cast<quint8>(MessageType::SessionClose); }

// ============================================================================
// 帧头结构（设计文档 §6.1）
//   Offset Size Field
//   0      4    Magic = 0x4C4E4654 ("LNFT")
//   4      1    Version
//   5      1    Type
//   6      2    Flags
//   8      2    HeaderLength
//   10     8    PayloadLength
//   18     8    MessageId
//   26     16   TransferId
// ============================================================================
struct PacketHeader {
    quint32 magic = PROTOCOL_MAGIC;
    quint8  version = PROTOCOL_VERSION;
    quint8  type = 0;                     // MessageType
    quint16 flags = 0;
    quint16 headerLength = HEADER_LENGTH;
    quint64 payloadLength = 0;
    quint64 messageId = 0;
    QUuid   transferId;                   // 16 字节会话 ID

    QByteArray encode() const;            // 编码为 42 字节大端字节串
    static bool decode(const char* data, qsizetype len, PacketHeader* out); // len >= HEADER_LENGTH
};

} // namespace lantransfer
