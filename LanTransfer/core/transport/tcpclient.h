#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

// ============================================================================
// TcpClient：发起 TCP 连接（设计文档 §10）
// 发送方作为 Client；连接成功/失败后 socket 所有权移交会话层。
// ============================================================================
namespace lantransfer {

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);

    void connectToHost(const QString& ip, quint16 port, int timeoutMs = 10000);

signals:
    // 连接成功；socket 所有权移交调用方（TransferManager）
    void connected(QTcpSocket* socket);
    void failed(const QString& error);

private:
    // 堆分配：连接成功后 socket 所有权移交给会话层（setParent(nullptr)），
    // TcpClient 自身销毁不得连带销毁 socket
    QTcpSocket* m_socket = nullptr;
    QTimer m_timeout;
    bool m_connecting = false;
};

} // namespace lantransfer
