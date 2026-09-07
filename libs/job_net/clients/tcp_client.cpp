#include "tcp_client.h"
#include <job_logger.h>

namespace job::net {

TcpClient::TcpClient(threads::JobIoAsyncThread::Ptr loop, JobResolver::Ptr resolver, uint16_t buffer_size) :
    m_loop(std::move(loop)),
    m_resolver(std::move(resolver)),
    m_readBuffer(buffer_size)
{
    m_socket = TcpSocket::create(m_loop);
    m_socket->setResolver(m_resolver);
    setupSocketCallbacks();
}

TcpClient::~TcpClient()
{
    disconnect();
}

bool TcpClient::connectToHost(const JobIpAddr &ipaddr)
{
    if (!m_socket) {
        m_socket = TcpSocket::create(m_loop);
        m_socket->setResolver(m_resolver);
        setupSocketCallbacks();
    }

    if (!m_socket->connectToHost(ipaddr)) {
        if (onError)
            onError(static_cast<int>(m_socket->lastError()));
        return false;
    }
    return true;
}

bool TcpClient::connectToHost(const JobUrl &url)
{
    if (!m_socket) {
        m_socket = TcpSocket::create(m_loop);
        m_socket->setResolver(m_resolver);
        setupSocketCallbacks();
    }

    if (!m_resolver) {
        JOB_LOG_ERROR("[TcpClient] connectToHost(url) requires a resolver — call setResolver() first");
        if (onError)
            onError(static_cast<int>(SocketErrors::SocketErrNo::Invalid));
        return false;
    }

    // ISocketIO::connectToHost(JobUrl) handles resolve-then-connect-per-candidate;
    // failures (resolution or every candidate rejected) surface via m_socket->onError,
    // already wired in setupSocketCallbacks(), not via this call's return value alone.
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

NetIoResult TcpClient::send(const void *data, size_t size)
{
    if (!m_socket || !isConnected())
        return NetIoResult::error();

    return m_socket->write(data, size);
}

NetIoResult TcpClient::send(const std::string &data)
{
    return send(data.data(), data.size());
}

bool TcpClient::setWriteInterest(bool enable)
{
    return m_socket && m_socket->setWriteInterest(enable);
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
    if (m_socket) {
        if (m_socket->isOpen())
            m_socket->disconnect();

        // Detach this client's callbacks from the outgoing socket so it can't keep
        // firing into a TcpClient that's moved on, if some other owner is still
        // holding a reference to it.
        m_socket->onConnect = nullptr;
        m_socket->onRead = nullptr;
        m_socket->onWrite = nullptr;
        m_socket->onDisconnect = nullptr;
        m_socket->onError = nullptr;
    }

    m_socket = std::move(socket);

    if (!m_socket) {
        m_connected.store(false, std::memory_order_relaxed);
        return;
    }

    m_socket->setResolver(m_resolver);
    m_connected.store(m_socket->state() == ISocketIO::SocketState::Connected, std::memory_order_relaxed);
    setupSocketCallbacks();
    // I see you ... you're already here and you matter :>)
}

void TcpClient::setResolver(JobResolver::Ptr resolver)
{
    m_resolver = std::move(resolver);
    if (m_socket)
        m_socket->setResolver(m_resolver);
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
        if (m_readBuffer.empty())
            return;

        // Drain: edge-triggered readability is reported once, so read until the
        // transport says there is nothing left.
        for (;;) {
            const NetIoResult n = m_socket->read(m_readBuffer.data(), m_readBuffer.size());

            if (n.ok()) {
                if (n.bytes == 0)
                    break; // Defensive: a stream read of a non-empty buffer never returns 0.

                if (onMessage)
                    onMessage(m_readBuffer.data(), n.bytes);

                continue;
            }

            // WouldBlock: drained for now.
            // Closed: orderly shutdown; TcpSocket::read() already called
            //         disconnect(), so onDisconnect will follow.
            // Error:   reported through onError by the socket layer.
            break;
        }
    };

    m_socket->onWrite = [this](const char *, size_t) {
        if (onWritable)
            onWritable();
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