#include "tcp_client.h"

namespace job::net {

TcpClient::TcpClient()
{
    m_loop = std::make_shared<threads::AsyncEventLoop>();
    m_socket = std::make_shared<TcpSocket>(m_loop);
}

TcpClient::TcpClient(const std::shared_ptr<threads::AsyncEventLoop> &loop)
    : m_loop(loop)
{
    m_socket = std::make_shared<TcpSocket>(m_loop);
}

TcpClient::~TcpClient()
{
    disconnect();
}

bool TcpClient::connectToHost(const std::string &host, uint16_t port)
{
    JobUrl url;
    url.setScheme("tcp");
    url.setHost(host);
    url.setPort(port);
    if(connectToHost(url)){
        // registerEvents();
        return true;
    }

    return false;
}

bool TcpClient::connectToHost(const JobUrl &url)
{
    m_socket = std::make_shared<TcpSocket>(m_loop);
    if (!m_socket->connectToHost(url)) {
        if (onError)
            onError(static_cast<int>(m_socket->lastError()));
        return false;
    }

    setupSocketCallbacks();
    m_socket->registerEvents(); // Looks like it already does that
    return true;
}

void TcpClient::disconnect()
{
    if (m_socket) {
        m_socket->disconnect();
        m_socket.reset();
        m_connected.store(false, std::memory_order_relaxed);
    }
}

ssize_t TcpClient::send(const void *data, size_t size)
{
    if (!m_socket || !isConnected())
        return -1;
    return m_socket->write(data, size);
}

ssize_t TcpClient::send(const std::string &data)
{
    return send(data.data(), data.size());
}

bool TcpClient::isConnected() const noexcept
{
    return m_connected.load(std::memory_order_relaxed);
}

SocketErrors::SocketErrNo TcpClient::lastError() const noexcept
{
    if (!m_socket)
        return SocketErrors::SocketErrNo::None;
    return m_socket->lastError();
}

std::string TcpClient::lastErrorString() const noexcept
{
    if (!m_socket)
        return "None";
    return m_socket->lastErrorString();
}

void TcpClient::setSocket(const std::shared_ptr<ISocketIO> &socket) {
    m_socket = std::dynamic_pointer_cast<TcpSocket>(socket);
    m_connected.store(true);
}

void TcpClient::registerEvents()
{
    if (m_socket) {
        m_socket->onRead = [this](const char *data, size_t len) {
            if (onMessage) {
                onMessage(data, len);
            }
        };

        m_socket->onDisconnect = [this]() {
            if (onDisconnect) {
                onDisconnect();
            }
        };

        m_socket->registerEvents();
    }
}

void TcpClient::setupSocketCallbacks()
{
    if (!m_socket)
        return;

    m_socket->onConnect = [this]() {
        m_connected.store(true, std::memory_order_relaxed);
        if (onConnect)
            onConnect();
    };

    m_socket->onRead = [this](const char *data, size_t len) {
        if (onMessage)
            onMessage(data, len);
    };

    m_socket->onDisconnect = [this]() {
        m_connected.store(false, std::memory_order_relaxed);
        if (onDisconnect)
            onDisconnect();
    };

    m_socket->onError = [this](int err) {
        m_connected.store(false, std::memory_order_relaxed);
        if (onError)
            onError(err);
    };
}

} // namespace job::net
