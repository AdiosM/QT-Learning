#pragma once

#include <QString>
#include <QUuid>
#include <QtGlobal>

#include "protocol/payloads.h"

// ============================================================================
// 传输任务与状态机类型（设计文档 §11）
//   Idle -> Connecting -> Handshaking -> WaitingUserAccept -> WaitingFileAccept
//        -> Transferring -> Verifying -> Completed
//        （侧向：Cancelling->Cancelled / Disconnected->Failed / Timeout->Failed /
//               VerifyFailed）
// ============================================================================
namespace lantransfer {

enum class TransferRole : quint8 {
    Sender = 0,
    Receiver = 1,
};

enum class TransferState : quint8 {
    Idle = 0,
    Connecting,          // TCP 连接中（发送方）
    Handshaking,         // HELLO 交换
    WaitingUserAccept,   // 等待接收方人工确认（PAIR_RESPONSE）
    WaitingFileAccept,   // 已发 FILE_INFO，等待 FILE_ACCEPT
    Transferring,        // FILE_DATA 传输中
    Verifying,           // 已发/已收 FILE_END，等待校验结论
    Completed,           // 校验一致，传输成功
    Cancelling,          // 取消中（已发 CANCEL，等待收尾）
    Cancelled,           // 已取消
    Disconnected,        // 连接断开
    Failed,              // 失败（含超时/拒绝/磁盘满/协议错等，见 task.errorCode）
    VerifyFailed,        // SHA-256 不一致
};

QString transferStateText(TransferState state);

// 接收端同名文件冲突处理（设计文档 §11）
enum class ConflictResolution : quint8 {
    AutoRename = 0, // 自动生成 "(1)" 等新名
    Overwrite = 1,  // 覆盖已有文件
    Cancel = 2,     // 取消本次传输
};

// 是否处于"任务仍在进行"的状态（用于活跃任务统计/单设备并发限制）
bool isTransferActive(TransferState state);

// 是否终态
bool isTransferFinished(TransferState state);

// 传输进度快照（TransferManager 定时刷新速度/ETA）
struct TransferProgress {
    quint64 totalBytes = 0;
    quint64 transferredBytes = 0;
    double speedBytesPerSec = 0.0; // 平滑速度
    qint64 elapsedMs = 0;
    qint64 remainingMs = -1;       // -1 表示未知
};

// 一个传输任务的完整描述
struct TransferTask {
    QUuid taskId;        // 管理器内部任务 ID（模型主键）
    QUuid transferId;    // 协议层会话 ID（建立连接后回填）
    QString deviceId;
    QString deviceName;   // 对端设备名
    QString fileName;
    quint64 fileSize = 0;
    TransferRole role = TransferRole::Sender;
    TransferState state = TransferState::Idle;
    ErrorCode errorCode = ErrorCode::Ok;
    QString errorText;
    QString finalPath;    // 接收完成后的落盘路径
    TransferProgress progress;
};

} // namespace lantransfer
