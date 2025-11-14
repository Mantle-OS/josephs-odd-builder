#include "udp_socket.h"

#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <job_logger.h>

#include <job_io_async_thread.h>

namespace job::net {

UdpSocket::UdpSocket(std::shared_ptr<threads::JobIoAsyncThread> loop):
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

UdpSocket::~UdpSocket()
{
    closeSocket();
}

void UdpSocket::closeSocket()
{
    // FIX: Always set state to Closed
    m_state.store(SocketState::Closed);
    if (m_fd < 0)
        return;

    if (auto loop = m_loop.lock())
        loop->unregisterFD(m_fd);

    ::close(m_fd);
    m_fd = -1;
}

void UdpSocket::updateLocalInfo() {
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getsockname(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_boundAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[UdpSocket] updateLocalInfo: Failed to parse local address.");
}

bool UdpSocket::connectToHost(const JobUrl &url)
{
    if (m_fd >= 0)
        closeSocket();

    JobIpAddr addr(url.host(), url.port());
    if (!addr.isValid()) {
        m_errors.setError(EINVAL);
        m_state.store(SocketState::Error);
        return false;
    }

    // Reset state to Unconnected if we're reusing a closed socket
    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(addr.family() == JobIpAddr::Family::IPv6 ? AF_INET6 : AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    if (::connect(m_fd, addr.sockAddr(), addr.sockAddrLen()) < 0) {
        m_errors.setError(errno);
        closeSocket();
        return false;
    }

    m_peerAddr = addr;
    m_state.store(SocketState::Connected);
    updateLocalInfo();

    registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
    return true;
}

bool UdpSocket::bind(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    // Reset state to Unconnected if we're reusing a closed socket
    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(addr.family() == JobIpAddr::Family::IPv6 ? AF_INET6 : AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    setOption(SocketOption::ReuseAddress, true);

    if (::bind(m_fd, addr.sockAddr(), addr.sockAddrLen()) < 0) {
        m_errors.setError(errno);
        closeSocket();
        return false;
    }

    updateLocalInfo(); // Get kernel-assigned port
    m_state.store(SocketState::Connected); // A bound UDP socket is "connected"

    registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
    return true;
}

bool UdpSocket::bind(const std::string &address, uint16_t port)
{
    JobIpAddr addr(address, port);
    return bind(addr);
}

bool UdpSocket::listen([[maybe_unused]]int backlog)
{
    // Not supported for UDP
    m_errors.setError(EOPNOTSUPP);
    return false;
}

std::shared_ptr<ISocketIO> UdpSocket::accept()
{
    // Not supported for UDP
    m_errors.setError(EOPNOTSUPP);
    return nullptr;
}

void UdpSocket::disconnect()
{
    if (m_fd >= 0)
        closeSocket();

    m_peerAddr.clear();
    m_state.store(SocketState::Closed); // This is already handled by closeSocket

    if(onDisconnect)
        onDisconnect();
}

ssize_t UdpSocket::read(void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    ssize_t ret = ::recv(m_fd, buffer, size, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // Not an error, just no data

        m_errors.setError(errno);
        return -1;
    }
    return ret;
}

ssize_t UdpSocket::write(const void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    ssize_t ret = ::send(m_fd, buffer, size, MSG_NOSIGNAL);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // Not an error, buffer is full

        m_errors.setError(errno);
        return -1;
    }
    return ret;
}

ssize_t UdpSocket::sendTo(const void *buffer, size_t size, const JobIpAddr &dest)
{
    if (m_fd < 0) {
        m_errors.setError(EBADF);
        return -1;
    }

    ssize_t sent = ::sendto(m_fd, buffer, size, 0,
                            dest.sockAddr(), dest.sockAddrLen());

    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // Not an error

        m_errors.setError(errno);
        return -1;
    }
    return sent;
}

ssize_t UdpSocket::recvFrom(void *buffer, size_t size, JobIpAddr &sender)
{
    if (m_fd < 0) {
        m_errors.setError(EBADF);
        return -1;
    }

    sockaddr_storage srcAddr{};
    socklen_t addrLen = sizeof(srcAddr);

    ssize_t received = ::recvfrom(m_fd, buffer, size, 0,
                                  reinterpret_cast<sockaddr *>(&srcAddr),
                                  &addrLen);
    if (received < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // Not an error

        m_errors.setError(errno);
        return -1;
    }

    if(!sender.fromSockAddr(reinterpret_cast<sockaddr*>(&srcAddr), addrLen))
        JOB_LOG_WARN("[UdpSocket] recvFrom: Failed to parse sender address.");

    return received;
}

SocketErrors::SocketErrNo UdpSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

ISocketIO::SocketType UdpSocket::type() const noexcept
{
    return SocketType::Udp;
}

ISocketIO::SocketState UdpSocket::state() const noexcept
{
    return m_state.load();
}

void UdpSocket::setOption(SocketOption option, bool enable)
{
    if (m_fd < 0)
        return;

    int value = enable ? 1 : 0;
    int ret = 0;

    switch (option) {
    case SocketOption::ReuseAddress:
        ret = ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
        break;
    case SocketOption::Broadcast:
        ret = ::setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
        break;
    case SocketOption::NonBlocking: {
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags >= 0)
            ret = fcntl(m_fd, F_SETFL, enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
        break;
    }
    default:
        // Other TCP-specific options are ignored
        break;
    }

    if (ret < 0)
        m_errors.setError(errno);
}

bool UdpSocket::option(SocketOption option) const
{
    if (m_fd < 0)
        return false;

    int value = 0;
    socklen_t len = sizeof(value);

    switch (option) {
    case SocketOption::ReuseAddress:
        ::getsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &value, &len);
        return value != 0;
    case SocketOption::Broadcast:
        ::getsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &value, &len);
        return value != 0;
    case SocketOption::NonBlocking: {
        int flags = fcntl(m_fd, F_GETFL, 0);
        return (flags & O_NONBLOCK) != 0;
    }
    default:
        return false;
    }
}

std::string UdpSocket::peerAddress() const
{
    return m_peerAddr.toString(false);
}

uint16_t UdpSocket::peerPort() const
{
    return m_peerAddr.port();
}

std::string UdpSocket::localAddress() const
{
    return m_boundAddr.toString(false);
}

uint16_t UdpSocket::localPort() const
{
    if (m_boundAddr.port() == 0 && m_fd != -1)
        const_cast<UdpSocket*>(this)->updateLocalInfo();

    return m_boundAddr.port();
}

void UdpSocket::dumpState() const
{
    JOB_LOG_DEBUG("[UdpSocket] fd={} state={} bound={}:{} peer={}:{}",
                  m_fd, (int)m_state.load(),
                  localAddress(), localPort(),
                  peerAddress(), peerPort()
                  );
}

bool UdpSocket::isOpen() const noexcept
{
    const auto current_state = m_state.load();
    return (current_state == ISocketIO::SocketState::Connected ||
            current_state == ISocketIO::SocketState::Listening ||
            current_state == ISocketIO::SocketState::Bound);
}

void UdpSocket::onEvents(uint32_t events)
{
    if (events & (POLLERR | POLLHUP)) {
        int error = 0;
        socklen_t len = sizeof(error);
        ::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
        m_errors.setError(error ? error : EIO);

        if (onError)
            onError(error ? error : EIO);

        if (events & POLLHUP)
            disconnect(); // HUP is more serious

        return;
    }

    if (events & EPOLLIN)
        if (onRead)
            onRead(nullptr, 0); // Signal that data is ready
}

} // namespace job::net
// CHECKPOINT: v2.1
