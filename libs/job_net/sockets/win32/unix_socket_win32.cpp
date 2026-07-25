#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <afunix.h>   // sockaddr_un, Win10 1803+ Wild I know never thought this day would come
#include <windows.h>

#include "sockets/unix_socket.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

#include <job_logger.h>
#include <io/job_io_async_thread.h>

#include "win_fd_reg.h"

namespace job::net {

UnixSocket::UnixSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

UnixSocket::UnixSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop, int existing_fd, const std::string &peerPath) :
    ISocketIO(std::move(loop)),
    m_peerPath(peerPath)
{
    m_fd = existing_fd;
    m_state.store(SocketState::Connected);
    setOption(SocketOption::NonBlocking, true);
    // Same reasoning as Linux: an accepted AF_UNIX stream socket's local address
    // (via getsockname) reflects the listener's own bound path, not anything
    // specific to this connection. m_ownsPath stays false, m_path stays empty.
}

UnixSocket::~UnixSocket()
{
    disconnect();
}

void UnixSocket::unlinkPath()
{
    // Windows AF_UNIX has no abstract-namespace concept (that's Linux-specific —
    // a leading '\0' byte there is never produced by anything on this platform),
    // so unlike the Linux side there's no '@'-prefix case to skip here — every
    // non-empty m_path on Windows is a real filesystem path.
    if (m_path.empty())
        return;

    if (!::DeleteFileA(m_path.c_str())) {
        const DWORD err = ::GetLastError();
        if (err != ERROR_FILE_NOT_FOUND)
            JOB_LOG_WARN("[UnixSocket] unlinkPath: DeleteFileA failed for '{}': Win32 Err {}", m_path, err);
    }
}

void UnixSocket::closeSocket()
{
    m_state.store(SocketState::Closed);

    if (m_fd < 0)
        return;

    if (auto loop = m_loop.lock())
        loop->unregisterFD(m_fd);

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock != INVALID_SOCKET)
        ::closesocket(nativeSock);

    threads::WinFdReg::instance().release(m_fd);
    m_fd = -1;
}

void UnixSocket::disconnect()
{
    auto expected = SocketState::Connected;
    if (m_state.compare_exchange_strong(expected, SocketState::Closing) ||
        m_state.load() == SocketState::Listening)
    {
        closeSocket();
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
        m_errors.setError(WSAEAFNOSUPPORT);
        return false;
    }

    if (!ipaddr.isValid()) {
        JOB_LOG_WARN("[UnixSocket] connectToHost called with invalid JobIpAddr");
        m_errors.setError(WSAEINVAL);
        return false;
    }

    if (m_fd >= 0)
        closeSocket();

    m_state.store(SocketState::Unconnected);

    const SOCKET nativeSock = ::socket(AF_UNIX, SOCK_STREAM, 0);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(::WSAGetLastError());
        m_state.store(SocketState::Error);
        return false;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(nativeSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        m_errors.setError(::WSAGetLastError());
        ::closesocket(nativeSock);
        m_state.store(SocketState::Error);
        return false;
    }

    m_fd = threads::WinFdReg::instance().allocate(nativeSock);

    if (m_fd < 0) {
        ::closesocket(nativeSock);
        m_errors.setError(WSAEMFILE);
        m_state.store(SocketState::Error);
        return false;
    }

    if (::connect(
            nativeSock,
            ipaddr.sockAddr(),
            static_cast<int>(ipaddr.sockAddrLen())) == SOCKET_ERROR) {

        const int lastError = ::WSAGetLastError();

        // Winsock 2 reports WSAEWOULDBLOCK when a nonblocking stream
        // connection cannot complete immediately. Completion is determined
        // through write/error readiness followed by SO_ERROR.
        //
        // Windows AF_UNIX should follow the same Winsock contract, but this
        // path requires validation on a native Windows system.
        //
        // WSAEALREADY (a second connect() already pending) shouldn't occur on
        // this first-attempt call path, but is accepted as pending defensively
        // rather than mis-recorded as a hard failure. WSAEINPROGRESS is
        // deliberately NOT treated as pending here — that's a Winsock 1.1
        // blocking-mode condition, not documented as a Winsock 2 nonblocking
        // connect() result.
        const bool connectionPending =
            lastError == WSAEWOULDBLOCK ||
            lastError == WSAEALREADY;

        if (connectionPending) {
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

        m_errors.setError(lastError);
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
            // weak_from_this() alone would yield weak_ptr<ISocketIO>
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
        m_errors.setError(WSAEINVAL);
        return false;
    }

    JobIpAddr addr(path);
    if (!addr.isValid() || addr.family() != JobIpAddr::Family::Unix) {
        JOB_LOG_ERROR("[UnixSocket] connectToHost(url): '{}' is not a valid Unix path", path);
        m_errors.setError(WSAEINVAL);
        return false;
    }

    return connectToHost(addr);
}

bool UnixSocket::bind(const JobIpAddr &addr)
{
    if (addr.family() != JobIpAddr::Family::Unix) {
        m_errors.setError(WSAEAFNOSUPPORT);
        return false;
    }

    if (!addr.isValid()) {
        m_errors.setError(WSAEINVAL);
        return false;
    }

    if (m_fd >= 0)
        closeSocket();

    const auto *un = reinterpret_cast<const sockaddr_un *>(addr.sockAddr());
    m_path = un->sun_path;

    // Clean any stale file left over from a previous run — independent of m_ownsPath,
    // same reasoning as the Linux side.
    unlinkPath();

    m_state.store(SocketState::Unconnected);

    const SOCKET nativeSock = ::socket(AF_UNIX, SOCK_STREAM, 0);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(::WSAGetLastError());
        m_state.store(SocketState::Error);
        return false;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(nativeSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        m_errors.setError(::WSAGetLastError());
        ::closesocket(nativeSock);
        m_state.store(SocketState::Error);
        return false;
    }

    m_fd = threads::WinFdReg::instance().allocate(nativeSock);

    if (m_fd < 0) {
        ::closesocket(nativeSock);
        m_errors.setError(WSAEMFILE);
        m_state.store(SocketState::Error);
        return false;
    }

    if (::bind(
            nativeSock,
            addr.sockAddr(),
            static_cast<int>(addr.sockAddrLen())) == SOCKET_ERROR) {

        m_errors.setError(::WSAGetLastError());
        closeSocket();
        return false;
    }

    m_ownsPath = true;
    return true;
}

bool UnixSocket::bind(const JobUrl &url)
{
    const std::string path = url.path();
    if (path.empty()) {
        m_errors.setError(WSAEINVAL);
        return false;
    }

    JobIpAddr addr(path);
    if (!addr.isValid() || addr.family() != JobIpAddr::Family::Unix) {
        JOB_LOG_ERROR("[UnixSocket] bind(url): '{}' is not a valid Unix path", path);
        m_errors.setError(WSAEINVAL);
        return false;
    }

    return bind(addr);
}

bool UnixSocket::listen(int backlog)
{
    if (m_fd < 0) {
        m_errors.setError(WSAENOTSOCK);
        return false;
    }

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(WSAENOTSOCK);
        return false;
    }

    if (::listen(nativeSock, backlog) == SOCKET_ERROR) {
        m_errors.setError(::WSAGetLastError());
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

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return nullptr;

    sockaddr_un client{};
    int addressLength = sizeof(client);

    const SOCKET clientSock = ::accept(
        nativeSock,
        reinterpret_cast<sockaddr *>(&client),
        &addressLength
        );

    if (clientSock == INVALID_SOCKET) {
        const int error = ::WSAGetLastError();

        if (error != WSAEWOULDBLOCK)
            m_errors.setError(error);

        return nullptr;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(clientSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        m_errors.setError(::WSAGetLastError());
        ::closesocket(clientSock);
        return nullptr;
    }

    const int clientFd = threads::WinFdReg::instance().allocate(clientSock);

    if (clientFd < 0) {
        ::closesocket(clientSock);
        m_errors.setError(WSAEMFILE);
        return nullptr;
    }

    auto loop = m_loop.lock();
    if (!loop) {
        threads::WinFdReg::instance().release(clientFd);
        ::closesocket(clientSock);
        return nullptr;
    }

    auto sock = UnixSocket::create(loop, clientFd, client.sun_path);
    sock->registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered);
    return sock;
}

ssize_t UnixSocket::read(void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    if (!buffer && size != 0) {
        m_errors.setError(WSAEFAULT);
        return -1;
    }

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return -1;

    const size_t boundedSize = std::min(
        size,
        static_cast<size_t>(std::numeric_limits<int>::max())
        );

    const int received = ::recv(
        nativeSock,
        static_cast<char *>(buffer),
        static_cast<int>(boundedSize),
        0
        );

    if (received == SOCKET_ERROR) {
        const int error = ::WSAGetLastError();

        if (error == WSAEWOULDBLOCK)
            return 0;

        m_errors.setError(error);
        return -1;
    }

    if (received == 0) {
        disconnect();
        return 0;
    }

    return static_cast<ssize_t>(received);
}

ssize_t UnixSocket::write(const void *buffer, size_t size)
{
    if (m_fd < 0)
        return -1;

    if (!buffer && size != 0) {
        m_errors.setError(WSAEFAULT);
        return -1;
    }

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return -1;

    const size_t boundedSize = std::min(
        size,
        static_cast<size_t>(std::numeric_limits<int>::max())
        );

    const int sent = ::send(
        nativeSock,
        static_cast<const char *>(buffer),
        static_cast<int>(boundedSize),
        0
        );

    if (sent == SOCKET_ERROR) {
        const int error = ::WSAGetLastError();

        if (error == WSAEWOULDBLOCK)
            return 0;

        m_errors.setError(error);
        return -1;
    }

    return static_cast<ssize_t>(sent);
}

SocketErrors::SocketErrNo UnixSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

void UnixSocket::triggerReadIfDataAvailable()
{
    if (m_fd < 0 || m_state.load() != SocketState::Connected)
        return;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return;

    char probe;
    const int n = ::recv(nativeSock, &probe, 1, MSG_PEEK);

    // Non-blocking peek: a genuine WSAEWOULDBLOCK just means "nothing available
    // yet," not an error worth recording here — same treatment as read()'s
    // ordinary drain path.
    if (n > 0 && onRead)
        onRead(nullptr, 0);
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
    const auto currentState = m_state.load();
    return currentState == SocketState::Connected ||
           currentState == SocketState::Listening;
}

void UnixSocket::setOption(SocketOption option, bool enable)
{
    if (option != SocketOption::NonBlocking || m_fd < 0)
        return;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return;

    u_long mode = enable ? 1UL : 0UL;

    if (::ioctlsocket(nativeSock, FIONBIO, &mode) == SOCKET_ERROR)
        m_errors.setError(::WSAGetLastError());
}

bool UnixSocket::option(SocketOption option) const
{
    // FIONBIO cannot be queried through getsockopt() — same limitation noted in
    // TcpSocket::option()/UdpSocket::option(). Unix domain sockets have no other
    // options this class exposes, so there's nothing else to report either way.
    (void)option;
    return false;
}

std::string UnixSocket::peerAddress() const
{
    return m_peerPath;
}

std::string UnixSocket::localAddress() const
{
    if (m_path.empty() && m_fd >= 0)
        const_cast<UnixSocket *>(this)->updateLocalInfo();

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
    JOB_LOG_DEBUG(
        "[UnixSocket] fd={} state={} path={} peer={}",
        m_fd,
        static_cast<int>(m_state.load()),
        m_path,
        m_peerPath
        );
}

void UnixSocket::updateLocalInfo()
{
    if (m_fd < 0)
        return;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return;

    sockaddr_un address{};
    int addressLength = sizeof(address);

    if (::getsockname(
            nativeSock,
            reinterpret_cast<sockaddr *>(&address),
            &addressLength) == SOCKET_ERROR) {
        return;
    }

    m_path = address.sun_path;
}

void UnixSocket::onEvents(threads::IOEvent events)
{
    if (threads::hasEvent(events, threads::IOEvent::Error) ||
        threads::hasEvent(events, threads::IOEvent::HangUp)) {
        const SOCKET nativeSock =
            threads::WinFdReg::instance().lookup(m_fd);
        int socketError = 0;
        int errorLength = sizeof(socketError);
        if (nativeSock != INVALID_SOCKET) {
            ::getsockopt(
                nativeSock,
                SOL_SOCKET,
                SO_ERROR,
                reinterpret_cast<char *>(&socketError),
                &errorLength
                );
        }

        if (socketError != 0) {
            // A genuine socket error occurred.
            m_errors.setError(socketError);
            if (onError)
                onError(socketError);
        }
        // else: HangUp with no real SO_ERROR just means the peer closed normally
        // not an error, don't report one. disconnect() below still fires onDisconnect
        // either way.

        disconnect();
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Write) &&
        m_state.load() == SocketState::Connecting) {

        const SOCKET nativeSock =
            threads::WinFdReg::instance().lookup(m_fd);

        int socketError = 0;
        int errorLength = sizeof(socketError);

        const bool connected =
            nativeSock != INVALID_SOCKET &&
            ::getsockopt(
                nativeSock,
                SOL_SOCKET,
                SO_ERROR,
                reinterpret_cast<char *>(&socketError),
                &errorLength) == 0 &&
            socketError == 0;

        if (connected) {
            m_state.store(SocketState::Connected);
            updateLocalInfo();

            modifyEvents(
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered
                );

            if (onConnect)
                onConnect();
        } else {
            const int reportedError =
                socketError != 0 ? socketError : WSAECONNABORTED;

            m_errors.setError(reportedError);

            if (onError)
                onError(reportedError);

            disconnect();
        }

        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Read) &&
        m_state.load() == SocketState::Listening) {

        while (true) {
            const SOCKET nativeSock =
                threads::WinFdReg::instance().lookup(m_fd);

            if (nativeSock == INVALID_SOCKET)
                break;

            sockaddr_un client{};
            int addressLength = sizeof(client);

            const SOCKET clientSock = ::accept(
                nativeSock,
                reinterpret_cast<sockaddr *>(&client),
                &addressLength
                );

            if (clientSock == INVALID_SOCKET) {
                const int error = ::WSAGetLastError();

                if (error == WSAEWOULDBLOCK)
                    break; // All connections drained

                m_errors.setError(error);
                break;
            }

            u_long nonBlocking = 1;

            if (::ioctlsocket(clientSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
                m_errors.setError(::WSAGetLastError());
                ::closesocket(clientSock);
                continue;
            }

            const int clientFd = threads::WinFdReg::instance().allocate(clientSock);

            if (clientFd < 0) {
                ::closesocket(clientSock);
                m_errors.setError(WSAEMFILE);
                continue;
            }

            auto loop = m_loop.lock();
            if (!loop) {
                JOB_LOG_ERROR("[UnixSocket] Event loop expired, closing fd={}", clientFd);
                threads::WinFdReg::instance().release(clientFd);
                ::closesocket(clientSock);
                continue;
            }

            auto sock = UnixSocket::create(loop, clientFd, client.sun_path);
            sock->registerEvents(
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered);

            if (onAccept) {
                onAccept(sock);
                if (sock->onRead)
                    sock->triggerReadIfDataAvailable();
            } else if (onConnect) {
                onConnect();
            }
        }
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Read) &&
        m_state.load() == SocketState::Connected) {
        if (onRead)
            onRead(nullptr, 0);
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Write) &&
        m_state.load() == SocketState::Connected) {
        JOB_LOG_DEBUG("[UnixSocket] Connected socket fd={} got write-ready", m_fd);
        return;
    }
}

} // namespace job::net