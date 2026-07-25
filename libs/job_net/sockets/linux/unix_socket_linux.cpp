#include "unix_socket.h"

#include <unistd.h>
#include <fcntl.h>

#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

#include <job_logger.h>

#include <job_io_async_thread.h>

namespace job::net {

UnixSocket::UnixSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

UnixSocket::UnixSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop, int existing_fd, const std::string& peerPath) :
    ISocketIO(std::move(loop)),
    m_peerPath(peerPath)
{
    m_fd = existing_fd;
    m_state.store(SocketState::Connected);
    setOption(SocketOption::NonBlocking, true);
    // Deliberately no updateLocalInfo() here: getsockname() on an accepted AF_UNIX
    // stream fd returns the *listener's* bound path, not anything specific to this
    // connection. m_ownsPath stays false, m_path stays empty for accepted sockets.
}

UnixSocket::~UnixSocket()
{
    disconnect();
}

void UnixSocket::unlinkPath()
{
    if (m_path.empty() || m_path[0] == '@')
        return; // empty, or abstract-namespace socket — nothing on the filesystem to remove
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
        // Only unlink if this socket itself bound the path — never for an accepted
        // child, whose m_path is empty anyway now, but keeping the explicit gate here
        // as the real fix for the original double-unlink bug (see conversation).
        if (m_ownsPath)
            unlinkPath();
        if (onDisconnect)
            onDisconnect();
    } else {
        closeSocket();
    }
}

bool UnixSocket::connectToHost(const JobIpAddr &ipaddr)
{
    if (ipaddr.family() != JobIpAddr::Family::Unix) {
        JOB_LOG_WARN("[UnixSocket] connectToHost: unsupported address family");
        m_errors.setError(EAFNOSUPPORT);
        return false;
    }

    if (!ipaddr.isValid()) {
        JOB_LOG_WARN("[UnixSocket] connectToHost called with invalid JobIpAddr");
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

    if (::connect(m_fd, ipaddr.sockAddr(), ipaddr.sockAddrLen()) < 0) {
        if (errno == EINPROGRESS) {
            m_state.store(SocketState::Connecting);
            m_peerPath = ipaddr.toString(false);
            registerEvents(
                threads::IOEvent::Write |
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered);
            return true;
        }

        m_errors.setError(errno);
        closeSocket();
        m_state.store(SocketState::Error);
        return false;
    }

    m_peerPath = ipaddr.toString(false);
    m_state.store(SocketState::Connected);
    updateLocalInfo();
    registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);

    if (onConnect) {
        if (auto loop = m_loop.lock()) {
            // weak_from_this() alone would yield weak_ptr<ISocketIO> — m_state is
            // private to UnixSocket, not visible through the base type. Recover the
            // derived pointer via static_pointer_cast before converting to weak_ptr;
            // safe since this is always a real UnixSocket when called from here.
            std::weak_ptr<UnixSocket> weakSelf = std::static_pointer_cast<UnixSocket>(shared_from_this());
            loop->post([weakSelf]() {
                auto self = weakSelf.lock();
                if (self && self->onConnect && self->m_state.load() == SocketState::Connected)
                    self->onConnect();
            });
        }
    }

    return true;
}
bool UnixSocket::connectToHost(const JobUrl &url)
{
    const std::string path = url.path();
    if (path.empty()) {
        m_errors.setError(EINVAL);
        return false;
    }

    JobIpAddr addr(path);
    if (!addr.isValid() || addr.family() != JobIpAddr::Family::Unix) {
        JOB_LOG_ERROR("[UnixSocket] connectToHost(url): '{}' is not a valid Unix path", path);
        m_errors.setError(EINVAL);
        return false;
    }

    return connectToHost(addr);
}

bool UnixSocket::bind(const JobIpAddr &addr)
{
    if (addr.family() != JobIpAddr::Family::Unix) {
        m_errors.setError(EAFNOSUPPORT);
        return false;
    }

    if (!addr.isValid()) {
        m_errors.setError(EINVAL);
        return false;
    }

    if (m_fd >= 0)
        closeSocket();

    const auto *un = reinterpret_cast<const sockaddr_un *>(addr.sockAddr());
    m_path = (un->sun_path[0] == '\0')
                 ? std::string("@") + (un->sun_path + 1)
                 : std::string(un->sun_path);

    // Clean any stale file left over from a previous run — independent of m_ownsPath,
    // since this is "clear leftover state before binding," not "I own this path now."
    // No-op for abstract sockets (unlinkPath() itself skips '@'-prefixed paths).
    unlinkPath();

    m_state.store(SocketState::Unconnected);

    m_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_fd < 0) {
        m_errors.setError(errno);
        m_state.store(SocketState::Error);
        return false;
    }

    if (::bind(m_fd, addr.sockAddr(), addr.sockAddrLen()) < 0) {
        m_errors.setError(errno);
        closeSocket();
        return false;
    }

    m_ownsPath = true; // this socket now owns m_path on disk — safe to unlink on disconnect
    return true;
}

bool UnixSocket::bind(const JobUrl &url)
{
    const std::string path = url.path();
    if (path.empty()) {
        m_errors.setError(EINVAL);
        return false;
    }

    JobIpAddr addr(path);
    if (!addr.isValid() || addr.family() != JobIpAddr::Family::Unix) {
        JOB_LOG_ERROR("[UnixSocket] bind(url): '{}' is not a valid Unix path", path);
        m_errors.setError(EINVAL);
        return false;
    }

    return bind(addr);
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
    registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);

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

    auto sock = UnixSocket::create(loop, cfd, client.sun_path);
    sock->registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);
    return sock;
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

    char probe;
    ssize_t n = ::recv(m_fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);

    if (n > 0 && onRead) {
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


void UnixSocket::onEvents(threads::IOEvent events)
{
    if (threads::hasEvent(events, threads::IOEvent::Error) || threads::hasEvent(events, threads::IOEvent::HangUp)) {
        int error = 0;
        socklen_t len = sizeof(error);
        ::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (error != 0) {
            // A genuine socket error occurred.
            m_errors.setError(error);
            if (onError)
                onError(error);
        }
        // else: HangUp with no real SO_ERROR just means the peer closed normally —
        // not an error, don't report one. disconnect() below still fires onDisconnect
        // either way.

        disconnect();
        return;
    }

    if ( job::threads::hasEvent(events, threads::IOEvent::Write) && m_state.load() == SocketState::Connecting) {
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
    if (job::threads::hasEvent(events, threads::IOEvent::Read) && m_state.load() == SocketState::Listening) {
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

            auto sock = UnixSocket::create(loop, cfd, client.sun_path);
            sock->registerEvents(
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered);
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

    if (job::threads::hasEvent(events, threads::IOEvent::Read) && m_state.load() == SocketState::Connected) {
        if (onRead)
            onRead(nullptr, 0);
        return;
    }

    if (job::threads::hasEvent(events, threads::IOEvent::Write) && m_state.load() == SocketState::Connected) {
        JOB_LOG_DEBUG("[UnixSocket] Connected socket fd={} got EPOLLOUT", m_fd);
        return;
    }
}
} // namespace job::net