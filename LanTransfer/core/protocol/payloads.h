#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

// ============================================================================
// 控制消息 JSON Payload 结构与错误码（设计文档 §6.2/§6.3/§11）
// 所有控制消息 Payload 为 UTF-8 JSON；FILE_DATA 为原始字节不经 JSON。
// ============================================================================
namespace lantransfer {

// 错误码（ERROR 帧 payload.code）
enum class ErrorCode : qint32 {
    Ok = 0,
    FileTooLarge = 1001,   // 文件超过 500,000,000 bytes 限制
    DiskFull = 1002,       // 磁盘空间不足
    Incomplete = 1003,     // 收到的字节数与 fileSize 不一致
    ProtocolError = 1004,  // 帧格式/状态机协议错误
    Timeout = 1005,        // 连接/握手/确认/数据空闲超时
    UserRejected = 1006,   // 对方拒绝传输
    Cancelled = 1007,      // 主动取消
    VerifyFailed = 1008,   // SHA-256 校验不一致
    InvalidName = 1009,    // 文件名非法
    Busy = 1010,           // 对端忙（已有活动任务）
    NotFound = 1011,       // 发送时文件不存在/不可读
    PinMismatch = 1012,    // PIN 不匹配
    Disconnected = 1013,   // TCP 连接断开
    InternalError = 1999,  // 内部错误
};

// 错误码 -> 中文可读说明（UI 展示用）
QString errorMessage(ErrorCode code);

// HELLO：协议版本、设备ID、能力协商
struct HelloPayload {
    QString deviceId;
    QString deviceName;
    QString deviceType;      // "windows" / "android"
    quint16 tcpPort = 0;     // 本端 TCP 监听端口
    QString appVersion;
    QStringList capabilities; // 如 ["file-transfer"]

    QJsonObject toJson() const;
    static HelloPayload fromJson(const QJsonObject& obj);
};

// PAIR_REQUEST：发送方请求允许传输
struct PairRequestPayload {
    QString deviceName;   // 发送方设备名（详情以 HELLO 为准）
    QString appVersion;
    QString fileName;     // 预展示用途，实际以 FILE_INFO 为准
    quint64 fileSize = 0;

    QJsonObject toJson() const;
    static PairRequestPayload fromJson(const QJsonObject& obj);
};

// PAIR_RESPONSE：ACCEPT/REJECT + 接收端生成的 6 位 PIN（辅助两端人工确认）
struct PairResponsePayload {
    bool accepted = false;
    QString pin;

    QJsonObject toJson() const;
    static PairResponsePayload fromJson(const QJsonObject& obj);
};

// FILE_INFO：文件元数据（设计文档 §6.3）
// V1.0 单文件只允许 basename；sha256 在初始握手时可空，
// 发送端流式算出的最终 SHA-256 随 FILE_END 发送。
struct FileInfoPayload {
    QString fileId;       // UUID 标识
    QString fileName;     // 仅 basename，UTF-8
    quint64 fileSize = 0; // 协议层 64 位；业务层限制 <= MAX_FILE_SIZE
    QString mimeType;     // 可选
    QString sha256;       // 可选（初始握手）
    QString relativePath; // V1.0 固定为空串

    QJsonObject toJson() const;
    static FileInfoPayload fromJson(const QJsonObject& obj);
};

// FILE_ACCEPT：接收方允许开始写入
struct FileAcceptPayload {
    bool accepted = false;

    QJsonObject toJson() const;
    static FileAcceptPayload fromJson(const QJsonObject& obj);
};

// FILE_END：发送方发送结束，携带边传边算的 SHA-256（设计文档 §8.3）
struct FileEndPayload {
    QString sha256;

    QJsonObject toJson() const;
    static FileEndPayload fromJson(const QJsonObject& obj);
};

// FILE_VERIFY：接收方校验结果
struct FileVerifyPayload {
    bool success = false;
    QString sha256;  // 接收端本地计算值

    QJsonObject toJson() const;
    static FileVerifyPayload fromJson(const QJsonObject& obj);
};

// CANCEL：主动取消（reason 为机器可读串，如 "user" / "pin_mismatch"）
struct CancelPayload {
    QString reason;

    QJsonObject toJson() const;
    static CancelPayload fromJson(const QJsonObject& obj);
};

// ERROR：错误码 + 可读说明
struct ErrorPayload {
    qint32 code = 0;
    QString message;

    QJsonObject toJson() const;
    static ErrorPayload fromJson(const QJsonObject& obj);
};

} // namespace lantransfer
