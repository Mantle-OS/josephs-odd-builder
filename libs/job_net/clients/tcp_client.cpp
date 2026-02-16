#include "tcp_client.h"
#include <job_logger.h>
namespace job::net {

TcpClient::TcpClient(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size) :
    m_loop(std::move(loop)),
    m_readBuffer(buffer_size)
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

    return connectToHost(url);
}

bool TcpClient::connectToHost(const JobUrl &url)
{
    if (!m_socket)
        m_socket = std::make_shared<TcpSocket>(m_loop);

    setupSocketCallbacks();

    if (!m_socket->connectToHost(url)) {
        if (onError)
            onError(static_cast<int>(m_socket->lastError()));

        return false;
    }

    return true;
}

void TcpClient::disconnect()
{
    if (m_socket) {
        m_socket->disconnect();
        // m_socket.reset(); that old saying "Reduce, Reuse, Recycle"
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

void TcpClient::setSocket(TcpSocket::Ptr socket)
{
    if (m_socket && m_socket->isOpen())
        m_socket->disconnect();

    m_socket = std::move(socket);
    m_connected.store(m_socket->state() == ISocketIO::SocketState::Connected);
    setupSocketCallbacks();

    // I see you ... you're already here and you matter :>)
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

    m_socket->onRead = [this]([[maybe_unused]] const char *data, [[maybe_unused]] size_t len) {
        while(true) {
            ssize_t n = m_socket->read(m_readBuffer.data(), m_readBuffer.size());
            if (n > 0) {
                if (onMessage)
                    onMessage(m_readBuffer.data(), n);
            } else if (n == 0) {
                break;
            } else {
                break;
            }
        }
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
// CHECKPOINT: v2.2
