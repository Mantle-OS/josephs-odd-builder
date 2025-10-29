#include "unix_socket.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

namespace job::net {

UnixSocket::UnixSocket(const std::shared_ptr<threads::AsyncEventLoop> &loop) :
    ISocketIO(loop)
{
    m_state.store(SocketState::Unconnected);
}

UnixSocket::~UnixSocket()
{
    disconnect();
}

void UnixSocket::unlinkPath()
{
    if (!m_path.empty()) {
        ::unlink(m_path.c_str());
    }
}

void UnixSocket::closeSocket()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_state.store(SocketState::Closed);
}

bool UnixSocket::bind([[maybe_unused]] const JobIpAddr &addr)
{
    // ignore JobIpAddr for unix sockets
    return false;
}

bool UnixSocket::bind(const std::string &path, uint16_t)
{
    closeSocket();
    unlinkPath(); // clean stale file

    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    sockaddr_un un{};
    un.sun_family = AF_UNIX;
    std::snprintf(un.sun_path, sizeof(un.sun_path), "%s", path.c_str());

    if (::bind(m_fd, reinterpret_cast<sockaddr *>(&un), sizeof(un)) < 0) {
        m_errors.setError(errno);
        closeSocket();
        return false;
    }

    m_path = path;
    setOption(SocketOption::NonBlocking, true);
    m_state.store(SocketState::Connected);
    return true;
}

bool UnixSocket::listen(int backlog)
{
    if (::listen(m_fd, backlog) < 0) {
        m_errors.setError(errno);
        return false;
    }
    return true;
}

std::shared_ptr<ISocketIO> UnixSocket::accept()
{
    sockaddr_un client{};
    socklen_t len = sizeof(client);
    int cfd = ::accept(m_fd, reinterpret_cast<sockaddr *>(&client), &len);

    if (cfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return nullptr; // no pending connections yet
        }
        m_errors.setError(errno);
        return nullptr;
    }

    auto sock = std::make_shared<UnixSocket>(m_loop.lock());
    sock->m_fd = cfd;
    sock->m_state.store(SocketState::Connected);
    sock->m_peerPath = client.sun_path;
    sock->setOption(SocketOption::NonBlocking, true);
    sock->registerEvents();
    return sock;
}

bool UnixSocket::connectToHost(const JobUrl &url)
{
    const std::string path = url.path();
    if (path.empty()) return false;

    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    sockaddr_un un{};
    un.sun_family = AF_UNIX;
    std::snprintf(un.sun_path, sizeof(un.sun_path), "%s", path.c_str());

    if (::connect(m_fd, reinterpret_cast<sockaddr *>(&un), sizeof(un)) < 0) {
        m_errors.setError(errno);
        closeSocket();
        m_state.store(SocketState::Error);
        return false;
    }

    m_peerPath = path;
    setOption(SocketOption::NonBlocking, true);
    m_state.store(SocketState::Connected);
    return true;
}

void UnixSocket::disconnect()
{
    closeSocket();
    unlinkPath();
}

ssize_t UnixSocket::read(void *buffer, size_t size)
{
    ssize_t n = ::recv(m_fd, buffer, size, 0);
    if (n < 0)
        m_errors.setError(errno);
    return n;
}

ssize_t UnixSocket::write(const void *buffer, size_t size)
{
    ssize_t n = ::send(m_fd, buffer, size, MSG_NOSIGNAL);
    if (n < 0)
        m_errors.setError(errno);
    return n;
}

SocketErrors::SocketErrNo UnixSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

ISocketIO::SocketState UnixSocket::state() const noexcept
{
    return m_state.load();
}

void UnixSocket::setOption(SocketOption option, bool enable)
{
    if (option == SocketOption::NonBlocking && m_fd >= 0) {
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags >= 0)
            fcntl(m_fd, F_SETFL, enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
    }
}

bool UnixSocket::option(SocketOption) const
{
    return false;
}

std::string UnixSocket::peerAddress() const { return m_peerPath; }
std::string UnixSocket::localAddress() const { return m_path; }

void UnixSocket::dumpState() const
{
    std::cout << "[UnixSocket] fd=" << m_fd
              << " state=" << static_cast<int>(m_state.load())
              << " path=" << m_path
              << " peer=" << m_peerPath
              << std::endl;
}

void UnixSocket::pollEvents()
{
    running.store(true, std::memory_order_relaxed);

    std::thread([self = shared_from_this()]() {
        auto socket = std::static_pointer_cast<UnixSocket>(self);
        struct pollfd pfd{};
        pfd.fd = socket->m_fd;
        pfd.events = POLLIN | POLLERR | POLLHUP;

        while (socket->running.load(std::memory_order_relaxed) &&
               socket->state() == SocketState::Connected) {
            int rc = ::poll(&pfd, 1, 100);
            if (rc > 0) {
                socket->handleEvents(pfd.revents);
            }
        }
    }).detach();
}

void UnixSocket::handleEvents(uint32_t events)
{
    if (events & (POLLERR | POLLHUP)) {
        if (onDisconnect)
            onDisconnect();
        return;
    }

    if (events & POLLIN) {
        char buf[4096];
        ssize_t n = ::recv(m_fd, buf, sizeof(buf), 0);
        if (n > 0 && onRead)
            onRead(buf, static_cast<size_t>(n));
    }
}

} // namespace job::net
