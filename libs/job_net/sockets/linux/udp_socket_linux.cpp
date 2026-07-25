#include "udp_socket.h"

#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <job_logger.h>

#include <job_io_async_thread.h>

namespace job::net {

UdpSocket::UdpSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop):
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

bool UdpSocket::connectToHost(const JobIpAddr &ipaddr)
{
    if (m_fd >= 0)
        closeSocket();

    if (!ipaddr.isValid()) {
        JOB_LOG_WARN("[UdpSocket] connectToHost called with invalid JobIpAddr");
        m_errors.setError(EINVAL);
        m_state.store(SocketState::Error);
        return false;
    }

    int family = AF_UNSPEC;
    switch (ipaddr.family()) {
    case JobIpAddr::Family::IPv4:
        family = AF_INET;
        break;
    case JobIpAddr::Family::IPv6:
        family = AF_INET6;
        break;
    default:
        // UdpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        JOB_LOG_WARN("[UdpSocket] connectToHost: unsupported address family");
        m_errors.setError(EAFNOSUPPORT);
        m_state.store(SocketState::Error);
        return false;
    }

    // Reset state to Unconnected if we're reusing a closed socket
    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    if (::connect(m_fd, ipaddr.sockAddr(), ipaddr.sockAddrLen()) < 0) {
        m_errors.setError(errno);
        closeSocket();
        return false;
    }

    m_peerAddr = ipaddr;
    m_state.store(SocketState::Connected);
    updateLocalInfo();

    registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);

    if (onConnect)
        onConnect();

    return true;
}

bool UdpSocket::bind(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    if (!addr.isValid()) {
        m_errors.setError(EINVAL);
        return false;
    }

    int family = AF_UNSPEC;
    switch (addr.family()) {
    case JobIpAddr::Family::IPv4:
        family = AF_INET;
        break;
    case JobIpAddr::Family::IPv6:
        family = AF_INET6;
        break;
    default:
        // UdpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        m_errors.setError(EAFNOSUPPORT);
        return false;
    }

    // Reset state to Unconnected if we're reusing a closed socket
    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
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
    // A bound-but-not-connect()'d UDP socket has no default peer — SocketState::Bound
    // is the correct state here, distinct from Connected (see conversation note).
    m_state.store(SocketState::Bound);

    registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);
    return true;
}

bool UdpSocket::bind(const JobUrl &url)
{
    // UDP binds to a local interface, never a remote peer — a hostname needing DNS
    // resolution has no sensible meaning here, so this deliberately never touches the
    // resolver. Only accepts an already-numeric IPv4/IPv6 literal. Same rule as
    // TcpSocket::bind(JobUrl).
    const std::string &host = url.host();

    if (!JobIpAddr::isIPv4(host) && !JobIpAddr::isIPv6(host)) {
        JOB_LOG_ERROR("[UdpSocket] bind(url) requires a numeric IPv4/IPv6 host, got '{}' — "
                      "hostname resolution is not performed for bind()", host);
        m_errors.setError(EINVAL);
        return false;
    }

    JobIpAddr addr(host, url.port());
    if (!addr.isValid()) {
        JOB_LOG_ERROR("[UdpSocket] bind(url): failed to construct address from '{}'", host);
        m_errors.setError(EINVAL);
        return false;
    }

    return bind(addr);
}

bool UdpSocket::listen([[maybe_unused]]int backlog)
{
    // Not supported for UDP
    m_errors.setError(EOPNOTSUPP);
    return false;
}

ISocketIO::Ptr UdpSocket::accept()
{
    // Not supported for UDP — connectionless, no per-client fd to hand off.
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

void UdpSocket::onEvents(job::threads::IOEvent events)
{
    if (threads::hasEvent(events, threads::IOEvent::Error) || threads::hasEvent(events, threads::IOEvent::HangUp)) {
        int error = 0;
        socklen_t len = sizeof(error);
        ::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (error != 0) {
            m_errors.setError(error);
            if (onError)
                onError(error);
        }
        // else: HangUp with no real SO_ERROR isn't an error worth reporting.

        if (threads::hasEvent(events, threads::IOEvent::HangUp))
            disconnect(); // HUP is more serious

        return;
    }

    if (job::threads::hasEvent(events, job::threads::IOEvent::Read))
        if (onRead)
            onRead(nullptr, 0); // Signal that data is ready
}

} // namespace job::net