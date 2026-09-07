#include "tcpserver.h"

namespace lantransfer {

TcpServer::TcpServer(QObject* parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, [this] {
        while (m_server.hasPendingConnections()) {
            QTcpSocket* socket = m_server.nextPendingConnection();
            socket->setParent(nullptr); // 所有权移交会话层
            emit incomingConnection(socket);
        }
    });
}

bool TcpServer::start(quint16 port)
{
    if (m_server.isListening())
        return true;
    m_error.clear();
    if (!m_server.listen(QHostAddress::Any, port)) {
        m_error = QStringLiteral("TCP 端口 %1 监听失败：%2").arg(port).arg(m_server.errorString());
        emit listenError(m_error);
        return false;
    }
    return true;
}

void TcpServer::stop()
{
    if (m_server.isListening())
        m_server.close();
}

} // namespace lantransfer
