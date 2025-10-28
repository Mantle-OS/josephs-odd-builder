#include "tcp_socket.h"

#include <unistd.h>
#include <cstring>
#include <iostream>

#include <fcntl.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <poll.h>

namespace job::net {

TcpSocket::TcpSocket(std::shared_ptr<threads::AsyncEventLoop> loop)
    : m_loop(loop)
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error, std::memory_order_relaxed);
        return;
    }

    m_fd = fd;
    setOption(SocketOption::NonBlocking, true);
    m_state.store(SocketState::Unconnected, std::memory_order_relaxed);
}

TcpSocket::~TcpSocket()
{
    disconnect();
}

bool TcpSocket::connectToHost(const JobUrl &url)
{
    bool ret = false;

    if (m_fd < 0)
        return ret;

    const std::string host = url.host();
    const uint16_t port = url.port();

    struct addrinfo hints{};
    struct addrinfo *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = ::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (err != 0) {
        m_errors.setError(errno);
        return ret;
    }

    for (auto *p = res; p != nullptr; p = p->ai_next) {
        if (::connect(m_fd, p->ai_addr, p->ai_addrlen) == 0 || errno == EINPROGRESS) {
            m_peerAddr = host;
            m_peerPort = port;
            m_state.store(SocketState::Connecting, std::memory_order_relaxed);
            registerEvents();
            ret = true;
            break;
        }
    }

    ::freeaddrinfo(res);
    return ret;
}

bool TcpSocket::bind(const JobIpAddr &addr)
{
    bool ret = false;

    if (m_fd < 0)
        return ret;

    const sockaddr *sa = addr.sockAddr();
    socklen_t len = addr.sockAddrLen();

    if (!sa || len == 0)
        return ret;

    if (::bind(m_fd, sa, len) == 0)
        ret = true;
    else
        m_errors.setError(errno);

    return ret;
}
bool TcpSocket::bind(const std::string &address, uint16_t port)
{
    JobIpAddr addr(address, port);
    bool ret = bind(addr);
    return ret;
}

bool TcpSocket::listen(int backlog)
{
    bool ret = false;

    if (m_fd < 0)
        return ret;

    if (::listen(m_fd, backlog) == 0) {
        m_state.store(SocketState::Connected, std::memory_order_relaxed);
        ret = true;
    } else {
        m_errors.setError(errno);
        m_state.store(SocketState::Error, std::memory_order_relaxed);
    }

    return ret;
}

std::shared_ptr<ISocketIO> TcpSocket::accept()
{
    std::shared_ptr<ISocketIO> ret;

    if (m_fd < 0)
        return ret;

    struct sockaddr_in client{};
    socklen_t len = sizeof(client);

    int clientFd = ::accept(m_fd, reinterpret_cast<sockaddr *>(&client), &len);
    if (clientFd < 0) {
        m_errors.setError(errno);
        return ret;
    }

    auto sock = std::make_shared<TcpSocket>(m_loop.lock());
    sock->m_fd = clientFd;
    sock->m_state.store(SocketState::Connected, std::memory_order_relaxed);
    sock->m_peerAddr = inet_ntoa(client.sin_addr);
    sock->m_peerPort = ntohs(client.sin_port);
    sock->setOption(SocketOption::NonBlocking, true);
    sock->registerEvents();

    ret = std::move(sock);
    return ret;
}

void TcpSocket::disconnect()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_state.store(SocketState::Closed, std::memory_order_relaxed);
}

ssize_t TcpSocket::read(void *buffer, size_t size)
{
    ssize_t ret = -1;

    if (m_fd < 0)
        return ret;

    ret = ::recv(m_fd, buffer, size, 0);
    if (ret <= 0) {
        if (ret == 0)
            m_state.store(SocketState::Closed, std::memory_order_relaxed);
        else
            m_errors.setError(errno);
    }

    return ret;
}

ssize_t TcpSocket::write(const void *buffer, size_t size)
{
    ssize_t ret = -1;

    if (m_fd < 0)
        return ret;

    ret = ::send(m_fd, buffer, size, MSG_NOSIGNAL);
    if (ret < 0)
        m_errors.setError(errno);

    return ret;
}

ISocketIO::SocketType TcpSocket::type() const noexcept
{
    return SocketType::Tcp;
}

ISocketIO::SocketState TcpSocket::state() const noexcept
{
    return m_state.load(std::memory_order_relaxed);
}

SocketErrors::SocketErrNo TcpSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

void TcpSocket::setOption(SocketOption option, bool enable)
{
    if (m_fd < 0)
        return;

    int ret = -1;
    switch (option) {
    case SocketOption::ReuseAddress: {
        int val = enable ? 1 : 0;
        ret = ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        break;
    }
    case SocketOption::KeepAlive: {
        int val = enable ? 1 : 0;
        ret = ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
        break;
    }
    case SocketOption::TcpNoDelay: {
        int val = enable ? 1 : 0;
        ret = ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
        break;
    }
    case SocketOption::NonBlocking: {
        int flags = ::fcntl(m_fd, F_GETFL, 0);
        if (flags >= 0) {
            if (enable)
                flags |= O_NONBLOCK;
            else
                flags &= ~O_NONBLOCK;
            ret = ::fcntl(m_fd, F_SETFL, flags);
        }
        break;
    }
    default:
        break;
    }

    if (ret < 0)
        m_errors.setError(errno);
}

bool TcpSocket::option(SocketOption option) const
{
    bool ret = false;

    if (m_fd < 0)
        return ret;

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
    default:
        return ret;
    }

    ret = (val != 0);
    return ret;
}

std::string TcpSocket::peerAddress() const
{
    return m_peerAddr;
}

uint16_t TcpSocket::peerPort() const
{
    return m_peerPort;
}

std::string TcpSocket::localAddress() const
{
    std::string ret;

    if (m_fd < 0)
        return ret;

    struct sockaddr_in sa{};
    socklen_t len = sizeof(sa);

    if (::getsockname(m_fd, reinterpret_cast<sockaddr *>(&sa), &len) == 0)
        ret = inet_ntoa(sa.sin_addr);

    return ret;
}

uint16_t TcpSocket::localPort() const
{
    uint16_t ret = 0;

    if (m_fd < 0)
        return ret;

    struct sockaddr_in sa{};
    socklen_t len = sizeof(sa);

    if (::getsockname(m_fd, reinterpret_cast<sockaddr *>(&sa), &len) == 0)
        ret = ntohs(sa.sin_port);

    return ret;
}

void TcpSocket::dumpState() const
{
    std::cout << "[TcpSocket] fd=" << m_fd
              << " state=" << static_cast<int>(m_state.load())
              << " peer=" << m_peerAddr << ":" << m_peerPort
              << std::endl;
}

int TcpSocket::fd() const noexcept
{
    return m_fd;
}

void TcpSocket::registerEvents()
{
    auto loop = m_loop.lock();
    if (!loop)
        return;

    loop->post([self = shared_from_this()] {
        std::thread([self]() {
            struct pollfd pfd{};
            pfd.fd = self->m_fd;
            pfd.events = POLLIN | POLLOUT | POLLERR | POLLHUP;

            while (self->state() == SocketState::Connecting || self->state() == SocketState::Connected) {
                int rc = ::poll(&pfd, 1, 100);
                if (rc > 0)
                    self->handleEvents(pfd.revents);
            }
        }).detach();
    });
}

void TcpSocket::handleEvents(uint32_t events)
{
    if (events & POLLIN)
        handleRead();

    if (events & POLLOUT && m_state.load() == SocketState::Connecting) {
        m_state.store(SocketState::Connected, std::memory_order_relaxed);
        if (onConnect)
            onConnect();
    }

    if (events & (POLLERR | POLLHUP)) {
        m_state.store(SocketState::Error, std::memory_order_relaxed);
        if (onDisconnect)
            onDisconnect();
    }
}

void TcpSocket::handleRead()
{
    char buf[4096];
    ssize_t n = ::recv(m_fd, buf, sizeof(buf), 0);

    if (n > 0) {
        if (onRead)
            onRead(buf, static_cast<size_t>(n));
    } else if (n == 0) {
        m_state.store(SocketState::Closed, std::memory_order_relaxed);
        if (onDisconnect)
            onDisconnect();
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            m_state.store(SocketState::Error, std::memory_order_relaxed);
            m_errors.setError(errno);
            if (onError)
                onError(errno);
        }
    }
}

void TcpSocket::postToLoop(std::function<void()> fn)
{
    if (auto loop = m_loop.lock())
        loop->post(std::move(fn));
}


} // namespace job::net
