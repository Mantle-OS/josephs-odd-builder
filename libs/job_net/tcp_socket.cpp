#include "tcp_socket.h"

#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <job_logger.h>

#include <job_io_async_thread.h>

namespace job::net {

TcpSocket::TcpSocket(threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

TcpSocket::TcpSocket(threads::JobIoAsyncThread::Ptr loop, int existing_fd, const JobIpAddr &peerAddr) :
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
    // FIX: Always set state to Closed, even if fd is invalid
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
    // Also check for Listening state
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

bool TcpSocket::connectToHost(const JobUrl &url)
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

    const std::string host = url.host();
    const uint16_t port = url.port();

    struct addrinfo hints{};
    struct addrinfo *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = ::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (err != 0) {
        m_errors.setError(EAI_FAIL);
        return false;
    }

    bool connected = false;
    for (auto *p = res; p != nullptr; p = p->ai_next) {
        m_fd = ::socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK, p->ai_protocol);
        if (m_fd < 0)
            continue;

        if (::connect(m_fd, p->ai_addr, p->ai_addrlen) == 0) {
            // Connected immediately
            m_state.store(SocketState::Connected);
            if (!m_peerAddr.fromSockAddr(p->ai_addr, p->ai_addrlen))
                JOB_LOG_WARN("[TcpSocket] connectToHost: Failed to parse peer address.");
            registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET); // Register for reading
            connected = true;
            break;
        }

        if (errno == EINPROGRESS) {
            m_state.store(SocketState::Connecting);

            if (!m_peerAddr.fromSockAddr(p->ai_addr, p->ai_addrlen))
                JOB_LOG_WARN("[TcpSocket] connectToHost: Failed to parse peer address.");

            registerEvents(EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
            connected = true;
            break;
        }

        ::close(m_fd);
        m_fd = -1;
    }

    ::freeaddrinfo(res);

    if (!connected)
        m_errors.setError(errno);

    return connected;
}

bool TcpSocket::bind(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    m_fd = ::socket(addr.family() == JobIpAddr::Family::IPv6 ?
                        AF_INET6 : AF_INET,
                    SOCK_STREAM | SOCK_NONBLOCK, 0
                    );
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

bool TcpSocket::bind(const std::string &address, uint16_t port)
{
    JobIpAddr addr(address, port);
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
    registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);

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

    auto sock = std::make_shared<TcpSocket>(loop, clientFd, peerAddr);
    sock->registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);

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

void TcpSocket::updateLocalInfo() {
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getsockname(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_localAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updateLocalInfo: Failed to parse local address.");

}

void TcpSocket::updatePeerInfo() {
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getpeername(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_peerAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updatePeerInfo: Failed to parse peer address.");
}


void TcpSocket::onEvents(uint32_t events)
{
    if ((events & EPOLLERR) || (events & EPOLLHUP)) {
        int error = 0;
        socklen_t len = sizeof(error);
        ::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
        m_errors.setError(error ? error : EIO); // Use EIO if error is 0
        if (onError)
            onError(error ? error : EIO);
        disconnect();
        return;
    }

    if ((events & EPOLLOUT) && m_state.load() == SocketState::Connecting) {
        int error = 0;
        socklen_t len = sizeof(error);
        if (::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
            m_state.store(SocketState::Connected);
            updateLocalInfo();
            updatePeerInfo();
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

    if (events & EPOLLIN) {
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
// CHECKPOINT: v2.3
