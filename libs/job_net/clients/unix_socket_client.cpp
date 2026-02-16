#include "unix_socket_client.h"
#include <job_logger.h>

namespace job::net {

UnixClient::UnixClient(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size) :
    m_loop(std::move(loop)),
    m_readBuffer(buffer_size)
{
    m_socket = std::make_shared<UnixSocket>(m_loop);
}

UnixClient::~UnixClient()
{
    disconnect();
}

bool UnixClient::connectToHost(const std::string &path)
{
    JobUrl url;
    url.setScheme("unix");
    url.setPath(path);

    return connectToHost(url);
}

bool UnixClient::connectToHost(const JobUrl &url)
{
    if (url.scheme() != JobUrl::Scheme::Unix) {
        JOB_LOG_ERROR("[UnixClient] Invalid URL scheme, must be 'unix:'");
        return false;
    }

    if (!m_socket)
        m_socket = std::make_shared<UnixSocket>(m_loop);

    setupSocketCallbacks();

    if (!m_socket->connectToHost(url)) {
        if (onError)
            onError(static_cast<int>(m_socket->lastError()));
        return false;
    }

    // Note: onConnect will be fired by the event loop when the non-blocking connect (EPOLLOUT) completes.
    return true;
}

void UnixClient::disconnect()
{
    if (m_socket) {
        m_socket->disconnect();
        m_connected.store(false, std::memory_order_relaxed);
    }
}

ssize_t UnixClient::send(const void *data, size_t size)
{
    if (!m_socket || !isConnected())
        return -1;

    return m_socket->write(data, size);
}

ssize_t UnixClient::send(const std::string &data)
{
    return send(data.data(), data.size());
}

bool UnixClient::isConnected() const noexcept
{
    return m_connected.load(std::memory_order_relaxed) && m_socket && m_socket->isOpen();
}

SocketErrors::SocketErrNo UnixClient::lastError() const noexcept
{
    if (!m_socket)
        return SocketErrors::SocketErrNo::None;

    return m_socket->lastError();
}

std::string UnixClient::lastErrorString() const noexcept
{
    if (!m_socket)
        return "None";

    return m_socket->lastErrorString();
}

void UnixClient::setSocket(UnixSocket::Ptr socket)
{
    if (m_socket && m_socket->isOpen())
        m_socket->disconnect();

    m_socket = std::move(socket);
    m_connected.store(m_socket->state() == ISocketIO::SocketState::Connected);
    setupSocketCallbacks();
}

void UnixClient::setupSocketCallbacks()
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
// CHECKPOINT: v1.2
