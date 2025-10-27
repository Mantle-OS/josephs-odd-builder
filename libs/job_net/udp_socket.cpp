#include "udp_socket.h"

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>

namespace job::net {

UdpSocket::UdpSocket()
{
    m_state.store(SocketState::Unconnected);
}

UdpSocket::~UdpSocket()
{
    closeSocket();
}

void UdpSocket::closeSocket()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_state.store(SocketState::Closed);
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

    m_fd = ::socket(addr.family() == JobIpAddr::Family::IPv6 ? AF_INET6 : AF_INET,
                    SOCK_DGRAM, 0);
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
    return true;
}

bool UdpSocket::bind(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    m_fd = ::socket(addr.family() == JobIpAddr::Family::IPv6 ? AF_INET6 : AF_INET,
                    SOCK_DGRAM, 0);
    if (m_fd < 0)
    {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    if (::bind(m_fd, addr.sockAddr(), addr.sockAddrLen()) < 0)
    {
        m_errors.setError(errno);
        closeSocket();
        return false;
    }

    // Retrieve actual bound address from kernel (for ephemeral ports, etc.)
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (::getsockname(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
    {
        JobIpAddr resolved;
        if (resolved.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            m_boundAddr = resolved;
        else
            m_boundAddr = addr; // fallback
    }
    else
    {
        m_boundAddr = addr; // fallback on failure
    }

    m_state.store(SocketState::Connected);
    return true;
}

bool UdpSocket::bind(const std::string &address, uint16_t port)
{
    JobIpAddr addr(address, port);
    return bind(addr);
}

void UdpSocket::disconnect()
{
    if (m_fd >= 0) {
        ::shutdown(m_fd, SHUT_RDWR);
        closeSocket();
    }
    m_peerAddr.clear();
    m_state.store(SocketState::Closed);
}

ssize_t UdpSocket::read(void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    ssize_t ret = ::recv(m_fd, buffer, size, 0);
    if (ret < 0) {
        m_errors.setError(errno);
        return -1;
    }

    return ret;
}

ssize_t UdpSocket::write(const void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    ssize_t ret = ::send(m_fd, buffer, size, 0);
    if (ret < 0) {
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
        m_errors.setError(errno);
        return -1;
    }


    if(!sender.fromSockAddr(reinterpret_cast<sockaddr*>(&srcAddr), addrLen)) {
        m_errors.setError(EINVAL);
        return -1;
    }
        // LOG

    return received;
}

SocketErrors::SocketErrNo UdpSocket::lastError() const noexcept
{
    return m_errors.lastError();
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

    switch (option) {
    case SocketOption::ReuseAddress:
        ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
        break;
    case SocketOption::Broadcast:
        ::setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
        break;
    case SocketOption::NonBlocking: {
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags >= 0)
            fcntl(m_fd, F_SETFL, enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
        break;
    }
    default:
        break;
    }
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
        return value;
    case SocketOption::Broadcast:
        ::getsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &value, &len);
        return value;
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
    return m_boundAddr.port();
}

void UdpSocket::dumpState() const
{
    std::cout << "[UdpSocket] fd=" << m_fd
              << " state=" << static_cast<int>(m_state.load())
              << " bound=" << m_boundAddr.toString(true)
              << " peer=" << m_peerAddr.toString(true)
              << " lastError=" << SocketErrors::toString(m_errors.lastError())
              << std::endl;
}

} // namespace job::net
