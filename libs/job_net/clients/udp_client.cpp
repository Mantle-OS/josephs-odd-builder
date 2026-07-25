#include "clients/udp_client.h"
#include <job_logger.h>
namespace job::net {

UdpClient::UdpClient(threads::JobIoAsyncThread::Ptr loop, JobResolver::Ptr resolver, uint16_t buffer_size) :
    m_loop(std::move(loop)),
    m_resolver(std::move(resolver)),
    m_readBuffer(buffer_size)
{
    m_socket = UdpSocket::create(m_loop);
    m_socket->setResolver(m_resolver);
    setupSocketCallbacks();
}

UdpClient::~UdpClient()
{
    disconnect();
}

bool UdpClient::connectToHost(const JobIpAddr &ipaddr)
{
    if (!m_socket) {
        m_socket = UdpSocket::create(m_loop);
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

bool UdpClient::connectToHost(const JobUrl &url)
{
    if (!m_socket) {
        m_socket = UdpSocket::create(m_loop);
        m_socket->setResolver(m_resolver);
        setupSocketCallbacks();
    }

    if (!m_resolver) {
        JOB_LOG_ERROR("[UdpClient] connectToHost(url) requires a resolver — call setResolver() first");
        if (onError)
            onError(static_cast<int>(SocketErrors::SocketErrNo::Invalid));
        return false;
    }

    // ISocketIO::connectToHost(JobUrl) handles resolve-then-connect; m_connected/onConnect
    // fire from m_socket->onConnect below once the underlying UdpSocket::connectToHost
    // (JobIpAddr) actually succeeds — not synchronously here, since resolution may not
    // have completed by the time this call returns.
    if (!m_socket->connectToHost(url)) {
        if (onError)
            onError(static_cast<int>(m_socket->lastError()));
        return false;
    }
    return true;
}

void UdpClient::disconnect()
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->disconnect();
        m_connected.store(false, std::memory_order_relaxed);
    }
}

ssize_t UdpClient::send(const void *data, size_t size)
{
    if (!m_socket || !isConnected())
        return -1;
    // "connected" ::send()
    return m_socket->write(data, size);
}

ssize_t UdpClient::send(const std::string &data)
{
    return send(data.data(), data.size());
}

ssize_t UdpClient::sendTo(const void *buffer, size_t size, const JobIpAddr &dest)
{
    if (!m_socket)
        return -1;
    return m_socket->sendTo(buffer, size, dest);
}

bool UdpClient::isConnected() const noexcept
{
    return m_connected.load(std::memory_order_relaxed);
}

SocketErrors::SocketErrNo UdpClient::lastError() const noexcept
{
    if (!m_socket)
        return SocketErrors::SocketErrNo::None;
    return m_socket->lastError();
}

std::string UdpClient::lastErrorString() const noexcept
{
    if (!m_socket)
        return "None";
    return m_socket->lastErrorString();
}

void UdpClient::setResolver(JobResolver::Ptr resolver)
{
    m_resolver = std::move(resolver);
    if (m_socket)
        m_socket->setResolver(m_resolver);
}

void UdpClient::setupSocketCallbacks()
{
    if (!m_socket)
        return;
    m_socket->onConnect = [this]() {
        m_connected.store(true, std::memory_order_relaxed);
        if (onConnect)
            onConnect();
    };
    m_socket->onRead = [this]([[maybe_unused]] const char *data, [[maybe_unused]] size_t len) {
        // DOWN the "drain"
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
        if (onError)
            onError(err);
    };
}

} // namespace job::net