#include "tcp_socket.h"

#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <sys/socket.h>

#include <job_logger.h>

#include <job_io_async_thread.h>

namespace job::net {

TcpSocket::TcpSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

TcpSocket::TcpSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop, int existing_fd, const JobIpAddr &peerAddr) :
    ISocketIO(std::move(loop)),
    m_peerAddr(peerAddr)
{
    m_fd = existing_fd;
    m_state.store(SocketState::Connected);
    setOption(SocketOption::NonBlocking, true);
    updateLocalInfo();
}


TcpSocket::~TcpSocket()
{
    disconnect();
}

void TcpSocket::closeSocket()
{
    m_state.store(SocketState::Closed);
    if (m_fd < 0)
        return;

    if (auto loop = m_loop.lock())
        loop->unregisterFD(m_fd);

    ::close(m_fd);
    m_fd = -1;
}

void TcpSocket::disconnect()
{
    auto expected = SocketState::Connected;
    if (m_state.compare_exchange_strong(expected, SocketState::Closing) ||
        m_state.load() == SocketState::Listening)
    {
        closeSocket();
        if (onDisconnect)
            onDisconnect();
    } else {
        // Already closing, unconnected, etc. Just ensure it's fully closed.
        closeSocket();
    }
}

bool TcpSocket::connectToHost(const JobIpAddr &ipaddr)
{
    if (m_fd >= 0)
        closeSocket();

    if (m_state.load() != SocketState::Unconnected) {
        // If state is Closed, reset to Unconnected to allow reuse
        if (m_state.load() == SocketState::Closed) {
            m_state.store(SocketState::Unconnected);
        } else {
            JOB_LOG_WARN("[TcpSocket] connectToHost called in invalid state: {}", (int)m_state.load());
            return false;
        }
    }

    if (!ipaddr.isValid()) {
        JOB_LOG_WARN("[TcpSocket] connectToHost called with invalid JobIpAddr");
        m_errors.setError(EINVAL);
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
        // TcpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        JOB_LOG_WARN("[TcpSocket] connectToHost: unsupported address family");
        m_errors.setError(EAFNOSUPPORT);
        return false;
    }

    m_fd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        return false;
    }

    m_peerAddr = ipaddr;
    m_state.store(SocketState::Connecting);

    if (::connect(m_fd, ipaddr.sockAddr(), ipaddr.sockAddrLen()) == 0) {
        // Connected immediately
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

    if (errno == EINPROGRESS) {
        registerEvents(
            threads::IOEvent::Write |
            threads::IOEvent::Read |
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered);
        return true;
    }

    m_errors.setError(errno);
    ::close(m_fd);
    m_fd = -1;
    m_state.store(SocketState::Unconnected);
    return false;
}

bool TcpSocket::bind(const JobIpAddr &addr)
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
        // TcpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        m_errors.setError(EAFNOSUPPORT);
        return false;
    }

    m_fd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        return false;
    }

    setOption(SocketOption::ReuseAddress, true);

    if (::bind(m_fd, addr.sockAddr(), addr.sockAddrLen()) != 0) {
        m_errors.setError(errno);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if(!m_localAddr.fromSockAddr(addr.sockAddr(), addr.sockAddrLen()))
        JOB_LOG_WARN("[TcpSocket] bind: Failed to copy local address.");

    return true;
}

bool TcpSocket::bind(const JobUrl &url)
{
    // TCP binds to a local interface, never a remote peer — a hostname needing DNS
    // resolution has no sensible meaning here, so this deliberately never touches the
    // resolver. Only accepts an already-numeric IPv4/IPv6 literal. (Unlike bind()'s
    // general JobIpAddr::isUnixPath() concept elsewhere in job_net, a Unix path is not
    // meaningful for TcpSocket specifically, that belongs to UnixSocket's own bind(JobUrl).)
    const std::string &host = url.host();

    if (!JobIpAddr::isIPv4(host) && !JobIpAddr::isIPv6(host)) {
        JOB_LOG_ERROR("[TcpSocket] bind(url) requires a numeric IPv4/IPv6 host, got '{}' — "
                      "hostname resolution is not performed for bind()", host);
        m_errors.setError(EINVAL);
        return false;
    }

    JobIpAddr addr(host, url.port());
    if (!addr.isValid()) {
        JOB_LOG_ERROR("[TcpSocket] bind(url): failed to construct address from '{}'", host);
        m_errors.setError(EINVAL);
        return false;
    }

    return bind(addr);
}

bool TcpSocket::listen(int backlog)
{
    if (m_fd < 0) {
        m_errors.setError(EBADF);
        return false;
    }

    if (::listen(m_fd, backlog) != 0) {
        m_errors.setError(errno);
        return false;
    }

    m_state.store(SocketState::Listening);
    registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);

    updateLocalInfo();
    return true;
}

ISocketIO::Ptr TcpSocket::accept()
{
    if (m_fd < 0 || m_state.load() != SocketState::Listening)
        return nullptr;


    sockaddr_storage client_addr{};
    socklen_t len = sizeof(client_addr);

    int clientFd = ::accept4(m_fd, reinterpret_cast<sockaddr *>(&client_addr), &len, SOCK_NONBLOCK);

    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            m_errors.setError(errno);

        return nullptr;
    }

    JobIpAddr peerAddr;
    if(!peerAddr.fromSockAddr(reinterpret_cast<sockaddr *>(&client_addr), len))
        JOB_LOG_WARN("[TcpSocket] accept: Failed to parse peer address.");

    auto loop = m_loop.lock();
    if (!loop) {
        ::close(clientFd);
        return nullptr;
    }

    auto sock = TcpSocket::create(loop, clientFd, peerAddr);
    sock->registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);

    return sock;
}

ssize_t TcpSocket::read(void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    ssize_t n = ::recv(m_fd, buffer, size, 0);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // Not an error

        m_errors.setError(errno);
        return -1;
    }
    if (n == 0) {
        // Peer closed connection
        disconnect();
        return 0;
    }
    return n;
}

ssize_t TcpSocket::write(const void *buffer, size_t size)
{
    if (m_fd < 0) return -1;

    std::lock_guard<std::mutex> lock(m_writeMutex);
    ssize_t n = ::send(m_fd, buffer, size, MSG_NOSIGNAL);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; // Not an error
        m_errors.setError(errno);
        return -1;
    }
    return n;
}

ISocketIO::SocketState TcpSocket::state() const noexcept
{
    return m_state.load();
}

SocketErrors::SocketErrNo TcpSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

std::string TcpSocket::lastErrorString() const noexcept
{
    return m_errors.lastErrorString();
}

ISocketIO::SocketType TcpSocket::type() const noexcept
{
    return SocketType::Tcp;
}

void TcpSocket::setOption(SocketOption option, bool enable)
{
    if (m_fd < 0) return;
    int ret = -1;
    int val = enable ? 1 : 0;

    switch (option) {
    case SocketOption::ReuseAddress:
        ret = ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        break;
    case SocketOption::KeepAlive:
        ret = ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
        break;
    case SocketOption::TcpNoDelay:
        ret = ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
        break;
    case SocketOption::NonBlocking: {
        int flags = ::fcntl(m_fd, F_GETFL, 0);
        if (flags >= 0) {
            flags = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
            ret = ::fcntl(m_fd, F_SETFL, flags);
        }
        break;
    }
    default:
        break;
    }
    if (ret < 0) m_errors.setError(errno);
}

bool TcpSocket::option(SocketOption option) const
{
    if (m_fd < 0) return false;
    int val = 0;
    socklen_t len = sizeof(val);

    switch (option) {
    case SocketOption::ReuseAddress:
        ::getsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, &len);
        break;
    case SocketOption::KeepAlive:
        ::getsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &val, &len);
        break;
    case SocketOption::TcpNoDelay:
        ::getsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, &len);
        break;
    case SocketOption::NonBlocking: {
        int flags = ::fcntl(m_fd, F_GETFL, 0);
        return (flags & O_NONBLOCK) != 0;
    }
    default:
        return false;
    }
    return (val != 0);
}

std::string TcpSocket::peerAddress() const
{
    return m_peerAddr.toString(false);
}
uint16_t TcpSocket::peerPort() const
{
    return m_peerAddr.port();
}
std::string TcpSocket::localAddress() const
{
    return m_localAddr.toString(false);
}
uint16_t TcpSocket::localPort() const
{
    if (m_localAddr.port() == 0 && m_fd != -1) {
        // const_cast is ugly, but this is a good place to lazily update
        const_cast<TcpSocket*>(this)->updateLocalInfo();
    }
    return m_localAddr.port();
}

void TcpSocket::dumpState() const
{
    JOB_LOG_DEBUG("[TcpSocket] fd={} state={} peer={}:{} local={}:{}",
                  m_fd, (int)m_state.load(),
                  peerAddress(), peerPort(),
                  localAddress(), localPort()
                  );
}

bool TcpSocket::isOpen() const noexcept
{
    const auto current_state = m_state.load();
    return (current_state == ISocketIO::SocketState::Connected ||
            current_state == ISocketIO::SocketState::Listening);
}

void TcpSocket::updateLocalInfo()
{
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getsockname(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_localAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updateLocalInfo: Failed to parse local address.");

}

void TcpSocket::updatePeerInfo()
{
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getpeername(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_peerAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updatePeerInfo: Failed to parse peer address.");
}


void TcpSocket::onEvents(threads::IOEvent events)
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
        // else: HangUp with no real SO_ERROR just means the peer closed normally —
        // not an error, don't report one. disconnect() below still fires onDisconnect.

        disconnect();
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Write) && m_state.load() == SocketState::Connecting) {
        int error = 0;
        socklen_t len = sizeof(error);
        if (::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
            m_state.store(SocketState::Connected);
            updateLocalInfo();
            updatePeerInfo();
            modifyEvents(
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered);
            if (onConnect)
                onConnect();
        } else {
            m_errors.setError(error ? error : EIO);
            if (onError)
                onError(error ? error : EIO);
            disconnect();
        }
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Read)) {
        if (m_state.load() == SocketState::Connected){
            if (onRead)
                onRead(nullptr, 0);
        }else if (m_state.load() == SocketState::Listening) {
            if (onConnect)
                onConnect();
        }
    }
}

} // namespace job::net