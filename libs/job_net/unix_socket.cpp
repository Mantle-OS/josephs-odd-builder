#include "unix_socket.h"

#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <job_logger.h>

#include <job_io_async_thread.h>

namespace job::net {

UnixSocket::UnixSocket(threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

UnixSocket::UnixSocket(threads::JobIoAsyncThread::Ptr loop, int existing_fd, const std::string& peerPath) :
    ISocketIO(std::move(loop)),
    m_peerPath(peerPath)
{
    m_fd = existing_fd;
    m_state.store(SocketState::Connected);
    setOption(SocketOption::NonBlocking, true);
    updateLocalInfo();
}

UnixSocket::~UnixSocket()
{
    disconnect();
}

void UnixSocket::unlinkPath()
{
    if (!m_path.empty() && m_state.load() != SocketState::Connecting)
        ::unlink(m_path.c_str());
}

void UnixSocket::closeSocket()
{
    m_state.store(SocketState::Closed);
    if (m_fd < 0)
        return;

    if (auto loop = m_loop.lock())
        loop->unregisterFD(m_fd);

    ::close(m_fd);
    m_fd = -1;
}

void UnixSocket::disconnect()
{
    auto expected = SocketState::Connected;
    if (m_state.compare_exchange_strong(expected, SocketState::Closing) ||
        m_state.load() == SocketState::Listening)
    {
        closeSocket();
        unlinkPath();
        if (onDisconnect)
            onDisconnect();
    } else {
        closeSocket();
    }
}

bool UnixSocket::bind([[maybe_unused]] const JobIpAddr &addr)
{
    m_errors.setError(EOPNOTSUPP);
    return false;
}

bool UnixSocket::bind(const std::string &path, [[maybe_unused]] uint16_t port)
{
    if (m_fd >= 0)
        closeSocket();

    m_path = path;
    unlinkPath(); // clean stale file

    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
    return true;
}

bool UnixSocket::listen(int backlog)
{
    if (m_fd < 0) {
        m_errors.setError(EBADF);
        return false;
    }

    if (::listen(m_fd, backlog) < 0) {
        m_errors.setError(errno);
        return false;
    }

    m_state.store(SocketState::Listening);
    registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);

    return true;
}

ISocketIO::Ptr UnixSocket::accept()
{
    if (m_fd < 0 || m_state.load() != SocketState::Listening)
        return nullptr;

    sockaddr_un client{};
    socklen_t len = sizeof(client);
    int cfd = ::accept4(m_fd, reinterpret_cast<sockaddr *>(&client), &len, SOCK_NONBLOCK);

    if (cfd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            m_errors.setError(errno);
        return nullptr; // why we always get hhere with the client.
    }

    auto loop = m_loop.lock();
    if (!loop) {
        ::close(cfd);
        return nullptr;
    }

    auto sock = std::make_shared<UnixSocket>(loop, cfd, client.sun_path);
    sock->registerEvents(EPOLLOUT |EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
    return sock;
}


bool UnixSocket::connectToHost(const JobUrl &url)
{
    const std::string path = url.path();
    if (path.empty()) {
        m_errors.setError(EINVAL);
        return false;
    }

    if (m_fd >= 0)
        closeSocket();

    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    sockaddr_un un{};
    un.sun_family = AF_UNIX;
    std::snprintf(un.sun_path, sizeof(un.sun_path), "%s", path.c_str());

    if (::connect(m_fd, reinterpret_cast<sockaddr *>(&un), sizeof(un)) < 0) {
        if (errno == EINPROGRESS) {
            m_state.store(SocketState::Connecting);
            m_peerPath = path;
            registerEvents(EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
            return true;
        }

        m_errors.setError(errno);
        closeSocket();
        m_state.store(SocketState::Error);
        return false;
    }
    m_peerPath = path;
    m_state.store(SocketState::Connected);
    updateLocalInfo();
    registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
    if (onConnect) {
        if (auto loop = m_loop.lock()) {
            loop->post([this]() {
                if (onConnect && m_state.load() == SocketState::Connected)
                    onConnect();
            });
        }
    }

    return true;
}



ssize_t UnixSocket::read(void *buffer, size_t size)
{
    ssize_t n = ::recv(m_fd, buffer, size, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        m_errors.setError(errno);
        return -1;
    }
    if (n == 0)
        disconnect();
    return n;
}

ssize_t UnixSocket::write(const void *buffer, size_t size)
{
    ssize_t n = ::send(m_fd, buffer, size, MSG_NOSIGNAL);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        m_errors.setError(errno);
        return -1;
    }
    return n;
}

SocketErrors::SocketErrNo UnixSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

void UnixSocket::triggerReadIfDataAvailable() {
    if (m_fd < 0 || m_state.load() != SocketState::Connected)
        return;

    // Peek at the socket to see if data is available without consuming it
    char probe;
    ssize_t n = ::recv(m_fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);

    if (n > 0 && onRead) {
        // Data is available, trigger the callback
        onRead(nullptr, 0);
    }
}

std::string UnixSocket::lastErrorString() const noexcept
{
    return m_errors.lastErrorString();
}

ISocketIO::SocketType UnixSocket::type() const noexcept
{
    return SocketType::Unix;
}

ISocketIO::SocketState UnixSocket::state() const noexcept
{
    return m_state.load();
}

bool UnixSocket::isOpen() const noexcept
{
    const auto current_state = m_state.load();
    return (current_state == ISocketIO::SocketState::Connected ||
            current_state == ISocketIO::SocketState::Listening);
}

void UnixSocket::setOption(SocketOption option, bool enable)
{
    if (option == SocketOption::NonBlocking && m_fd >= 0) {
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (flags >= 0)
            fcntl(m_fd, F_SETFL, enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
    }
}

bool UnixSocket::option(SocketOption option) const
{
    if (option == SocketOption::NonBlocking && m_fd >= 0) {
        int flags = fcntl(m_fd, F_GETFL, 0);
        return (flags & O_NONBLOCK) != 0;
    }
    return false;
}

std::string UnixSocket::peerAddress() const
{
    return m_peerPath;
}

std::string UnixSocket::localAddress() const
{
    // Lazily update local path info if it's empty
    if (m_path.empty() && m_fd != -1) {
        const_cast<UnixSocket*>(this)->updateLocalInfo();
    }
    return m_path;
}

uint16_t UnixSocket::peerPort() const
{
    return 0;
}

uint16_t UnixSocket::localPort() const
{
    return 0;
}

void UnixSocket::dumpState() const
{
    JOB_LOG_DEBUG("[UnixSocket] fd={} state={} path={} peer={}",
                  m_fd, (int)m_state.load(), m_path, m_peerPath
                  );
}

void UnixSocket::updateLocalInfo() {
    sockaddr_un sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getsockname(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        m_path = sa.sun_path;
}


void UnixSocket::onEvents(uint32_t events)
{
    if ((events & EPOLLERR) || (events & EPOLLHUP)) {
        int error = 0;
        socklen_t len = sizeof(error);
        ::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
        m_errors.setError(error ? error : EIO);
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

    // What a PITA
    if ((events & EPOLLIN) && m_state.load() == SocketState::Listening) {
        while (true) {
            sockaddr_un client{};
            socklen_t len = sizeof(client);
            int cfd = ::accept4(m_fd, reinterpret_cast<sockaddr *>(&client), &len, SOCK_NONBLOCK);

            if (cfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break; // All connections drained

                m_errors.setError(errno);
                break;
            }
            auto loop = m_loop.lock();
            if (!loop) {
                JOB_LOG_ERROR("[UnixSocket] Event loop expired, closing fd={}", cfd);
                ::close(cfd);
                continue;
            }

            auto sock = std::make_shared<UnixSocket>(loop, cfd, client.sun_path);
            sock->registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
            if (onAccept) {
                onAccept(sock);
                if (sock->onRead) {
                    pollfd pfd{};
                    pfd.fd = cfd;
                    pfd.events = POLLIN;
                    pfd.revents = 0;

                    int poll_result = ::poll(&pfd, 1, 0);
                    if (poll_result > 0 && (pfd.revents & POLLIN))
                        sock->onRead(nullptr, 0);
                    else if (poll_result < 0)
                        JOB_LOG_ERROR("[UnixSocket] poll() failed for fd={}: {}", cfd, strerror(errno));
                }
            } else if (onConnect)
                onConnect();
        }
        return;
    }

    if ((events & EPOLLIN) && m_state.load() == SocketState::Connected) {
        if (onRead)
            onRead(nullptr, 0);
        return;
    }

    if ((events & EPOLLOUT) && m_state.load() == SocketState::Connected) {
        JOB_LOG_DEBUG("[UnixSocket] Connected socket fd={} got EPOLLOUT", m_fd);
        // Add writer here
        return;
    }
}
} // namespace job::net
// CHECKPOINT: v2.3
