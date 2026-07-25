#include "udp_server.h"
#include <job_logger.h>
namespace job::net {
UdpServer::UdpServer(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size):
    m_loop(std::move(loop)),
    m_readBuffer(buffer_size)
{
    m_socket = UdpSocket::create(m_loop);
}
UdpServer::~UdpServer()
{
    stop();
}
bool UdpServer::start(const JobIpAddr &addr)
{
    if (!m_socket)
        return false;
    if (!m_socket->bind(addr))
        return false;
    m_port = m_socket->localPort();
    setupSocketCallbacks();
    return true;
}
bool UdpServer::start(const JobUrl &url)
{
    if (!m_socket)
        return false;
    if (!m_socket->bind(url))
        return false;
    m_port = m_socket->localPort();
    setupSocketCallbacks();
    return true;
}
void UdpServer::stop()
{
    if (m_socket && m_socket->isOpen())
        m_socket->disconnect();
}
uint16_t UdpServer::port() const noexcept
{
    return m_port;
}
bool UdpServer::isRunning() const noexcept
{
    return m_socket && m_socket->isOpen();
}
ssize_t UdpServer::sendTo(const void *buffer, size_t size, const JobIpAddr &dest)
{
    if (!isRunning())
        return -1;
    return m_socket->sendTo(buffer, size, dest);
}
void UdpServer::setupSocketCallbacks()
{
    if (!m_socket)
        return;
    m_socket->onRead = [this]([[maybe_unused]] const char *data, [[maybe_unused]] size_t len) {
        while (true) {
            JobIpAddr sender;
            ssize_t n = m_socket->recvFrom(m_readBuffer.data(), m_readBuffer.size(), sender);
            if (n > 0) {
                if (onMessage)
                    onMessage(m_readBuffer.data(), n, sender);
            } else if (n == 0) {
                break;
            } else {
                if (m_socket->lastError() == SocketErrors::SocketErrNo::WouldBlock)
                    break;
                JOB_LOG_ERROR("[UdpServer] recvFrom error: {}", m_socket->lastErrorString());
                if (onError)
                    onError(static_cast<int>(m_socket->lastError()));
                break;
            }
        }
    };
    m_socket->onError = [this](int err) {
        JOB_LOG_ERROR("[UdpServer] Socket error: {}", err);
        if (onError)
            onError(err);
    };
}
} // namespace job::net