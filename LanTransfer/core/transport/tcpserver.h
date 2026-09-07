#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "protocol/packet.h"

// ============================================================================
// TcpServer：TCP 监听（设计文档 §10）
// 两端均启动 TCP Server；"电脑发手机/手机发电脑"中接收方作为 Server。
// 每个传输任务一条独立连接，由 TransferManager 转成 TransferSession。
// ============================================================================
namespace lantransfer {

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QObject* parent = nullptr);

    // 启动监听；失败（如端口占用）返回 false，errorString() 给出原因
    bool start(quint16 port = DEFAULT_TCP_PORT);
    void stop();

    bool isListening() const { return m_server.isListening(); }
    quint16 port() const { return m_server.serverPort(); }
    QString errorString() const { return m_error; }

signals:
    // 新入站连接；socket 所有权移交接收方（TransferManager）
    void incomingConnection(QTcpSocket* socket);
    void listenError(const QString& message);

private:
    QTcpServer m_server;
    QString m_error;
};

} // namespace lantransfer
