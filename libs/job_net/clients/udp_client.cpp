#include "clients/udp_client.h"
#include <job_logger.h>

namespace job::net {

UdpClient::UdpClient(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size) :
    m_loop(std::move(loop)),
    m_readBuffer(buffer_size)
{
    m_socket = std::make_shared<UdpSocket>(m_loop);
}

UdpClient::~UdpClient()
{
    disconnect();
}

bool UdpClient::connectToHost(const std::string &host, uint16_t port)
{
    JobUrl url;
    url.setScheme("udp");
    url.setHost(host);
    url.setPort(port);

    return connectToHost(url);
}

bool UdpClient::connectToHost(const JobUrl &url)
{
    if (!m_socket)
        m_socket = std::make_shared<UdpSocket>(m_loop);

    setupSocketCallbacks();

    if (!m_socket->connectToHost(url)) {
        if (onError)
            onError(static_cast<int>(m_socket->lastError()));

        return false;
    }

    // Why I buy bunn coffee makers I want my coffee NOW !
    // just like UDP
    m_connected.store(true);
    if (onConnect)
        onConnect();

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

void UdpClient::setupSocketCallbacks()
{
    if (!m_socket)
        return;

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
// CHECKPOINT: v1.1
