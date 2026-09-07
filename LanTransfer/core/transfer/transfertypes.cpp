#include "transfertypes.h"

namespace lantransfer {

QString transferStateText(TransferState state)
{
    switch (state) {
    case TransferState::Idle:              return QStringLiteral("空闲");
    case TransferState::Connecting:        return QStringLiteral("连接中");
    case TransferState::Handshaking:       return QStringLiteral("握手");
    case TransferState::WaitingUserAccept: return QStringLiteral("等待对方确认");
    case TransferState::WaitingFileAccept: return QStringLiteral("等待接收确认");
    case TransferState::Transferring:      return QStringLiteral("传输中");
    case TransferState::Verifying:         return QStringLiteral("校验中");
    case TransferState::Completed:         return QStringLiteral("完成");
    case TransferState::Cancelling:        return QStringLiteral("取消中");
    case TransferState::Cancelled:         return QStringLiteral("已取消");
    case TransferState::Disconnected:      return QStringLiteral("连接断开");
    case TransferState::Failed:            return QStringLiteral("失败");
    case TransferState::VerifyFailed:      return QStringLiteral("校验失败");
    }
    return QStringLiteral("未知");
}

bool isTransferActive(TransferState state)
{
    switch (state) {
    case TransferState::Connecting:
    case TransferState::Handshaking:
    case TransferState::WaitingUserAccept:
    case TransferState::WaitingFileAccept:
    case TransferState::Transferring:
    case TransferState::Verifying:
    case TransferState::Cancelling:
        return true;
    default:
        return false;
    }
}

bool isTransferFinished(TransferState state)
{
    return !isTransferActive(state) && state != TransferState::Idle;
}

} // namespace lantransfer
