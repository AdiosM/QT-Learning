#pragma once

#include "packet.h"

#include <QByteArray>
#include <QJsonObject>
#include <optional>

// ============================================================================
// PacketCodec：帧组装与 JSON 控制消息序列化
// 职责边界（设计文档 §8.2）：只负责组帧/拆帧，不做业务状态判断。
// ============================================================================
namespace lantransfer {

class PacketCodec
{
public:
    // 组装完整帧（header + payload）；payload 超过 MAX_FRAME_PAYLOAD 时返回空
    static QByteArray encodeFrame(const PacketHeader& header, const QByteArray& payload);

    // 常用便捷路径：控制帧（JSON payload）
    static QByteArray encodeControl(MessageType type, const QUuid& transferId,
                                    const QJsonObject& payload,
                                    quint64 messageId = 0);

    // 数据帧（FILE_DATA，原始字节）
    static QByteArray encodeFileData(const QUuid& transferId, const QByteArray& chunk,
                                     quint64 messageId = 0);

    // 解析控制帧 payload 为 JSON 对象；非 JSON 时返回 nullopt
    static std::optional<QJsonObject> decodeControlJson(const QByteArray& payload);

    // 单调递增 MessageId（进程内线程安全）
    static quint64 nextMessageId();
};

} // namespace lantransfer
