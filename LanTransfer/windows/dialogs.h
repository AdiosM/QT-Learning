#pragma once

#include <QDialog>

#include "transfer/transfertypes.h"

// ============================================================================
// Windows 端对话框集：接收确认(含PIN) / PIN输入 / 文件冲突 / 手动IP连接
// 均为非模态使用，由 MainWindow 按会话管理生命周期。
// ============================================================================
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QDialogButtonBox;

// 接收确认对话框：展示对方设备/文件/6位PIN，用户选择接受或拒绝
class IncomingPairDialog : public QDialog
{
    Q_OBJECT
public:
    IncomingPairDialog(const QString& peerName, const QString& fileName,
                       quint64 fileSize, const QString& pin, QWidget* parent = nullptr);

signals:
    void acceptedPair(bool accepted);
};

// PIN 输入对话框（发送方）：要求输入接收端屏幕上显示的 6 位 PIN
class PinInputDialog : public QDialog
{
    Q_OBJECT
public:
    PinInputDialog(const QString& peerName, QWidget* parent = nullptr);

signals:
    void pinEntered(const QString& pin);
};

// 文件冲突对话框：覆盖 / 自动重命名 / 取消
class ConflictDialog : public QDialog
{
    Q_OBJECT
public:
    ConflictDialog(const QString& fileName, QWidget* parent = nullptr);

signals:
    void resolved(lantransfer::ConflictResolution resolution);
};

// 手动连接对话框：IP + TCP 端口（端口 0 表示通过 UDP 探测自动获取）
class ManualConnectDialog : public QDialog
{
    Q_OBJECT
public:
    ManualConnectDialog(QWidget* parent = nullptr);

    QString ip() const;
    quint16 tcpPort() const; // 0 = 自动探测

private:
    QLineEdit* m_ipEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
};
