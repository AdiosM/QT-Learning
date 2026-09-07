#include "tcpclient.h"

namespace lantransfer {

TcpClient::TcpClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    m_timeout.setSingleShot(true);

    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (!m_connecting)
            return;
        m_connecting = false;
        m_socket->abort();
        emit failed(QStringLiteral("连接超时"));
    });

    connect(m_socket, &QTcpSocket::connected, this, [this] {
        if (!m_connecting)
            return;
        m_connecting = false;
        m_timeout.stop();
        m_socket->setParent(nullptr); // 所有权移交会话层
        emit connected(m_socket);
    });

    connect(m_socket, &QAbstractSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
        if (!m_connecting)
            return;
        m_connecting = false;
        m_timeout.stop();
        emit failed(QStringLiteral("连接失败：%1").arg(m_socket->errorString()));
    });
}

void TcpClient::connectToHost(const QString& ip, quint16 port, int timeoutMs)
{
    if (m_connecting) {
        m_connecting = false;
        m_socket->abort();
        m_timeout.stop();
    }
    m_connecting = true;
    m_socket->connectToHost(ip, port);
    m_timeout.start(timeoutMs);
}

} // namespace lantransfer
