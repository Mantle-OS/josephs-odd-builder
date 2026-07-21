#include "tcp_socket.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <algorithm>

#include <job_logger.h>
#include <job_io_async_thread.h>

namespace job::net {

// Helper macros to cleanly map between public header int configurations and native Winsock handles
inline SOCKET toWinSock(int fd) noexcept { return static_cast<SOCKET>(static_cast<intptr_t>(fd)); }
inline int toJobFd(SOCKET sock) noexcept { return static_cast<int>(static_cast<intptr_t>(sock)); }

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
    m_state.store(SocketState::Closed);
    if (m_fd < 0)
        return;

    if (auto loop = m_loop.lock())
        loop->unregisterFD(m_fd);

    ::closesocket(toWinSock(m_fd));
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
        closeSocket();
    }
}

bool TcpSocket::connectToHost(const JobUrl &url)
{
    if (m_fd >= 0)
        closeSocket();

    if (m_state.load() != SocketState::Unconnected) {
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
        m_errors.setError(WSAHOST_NOT_FOUND);
        return false;
    }

    bool connected = false;
    for (auto *p = res; p != nullptr; p = p->ai_next) {
        SOCKET s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET)
            continue;

        m_fd = toJobFd(s);

        // Win32 lacks atomic non-blocking flags inside socket(). Flip state instantly.
        u_long nonBlocking = 1;
        ::ioctlsocket(s, FIONBIO, &nonBlocking);

        if (::connect(s, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
            m_state.store(SocketState::Connected);
            if (!m_peerAddr.fromSockAddr(p->ai_addr, static_cast<int>(p->ai_addrlen)))
                JOB_LOG_WARN("[TcpSocket] connectToHost: Failed to parse peer address.");

            registerEvents(
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered);
            connected = true;
            break;
        }

        int lastError = ::WSAGetLastError();
        if (lastError == WSAEWOULDBLOCK) {
            m_state.store(SocketState::Connecting);

            if (!m_peerAddr.fromSockAddr(p->ai_addr, static_cast<int>(p->ai_addrlen)))
                JOB_LOG_WARN("[TcpSocket] connectToHost: Failed to parse peer address.");

            registerEvents(
                threads::IOEvent::Write |
                threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered);
            connected = true;
            break;
        }

        ::closesocket(s);
        m_fd = -1;
    }

    ::freeaddrinfo(res);

    if (!connected)
        m_errors.setError(::WSAGetLastError());

    return connected;
}

bool TcpSocket::bind(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    SOCKET s = ::socket(addr.family() == JobIpAddr::Family::IPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        m_errors.setError(::WSAGetLastError());
        return false;
    }

    m_fd = toJobFd(s);

    // Enforce immediate asynchronous execution boundaries
    u_long nonBlocking = 1;
    ::ioctlsocket(s, FIONBIO, &nonBlocking);

    setOption(SocketOption::ReuseAddress, true);

    if (::bind(s, addr.sockAddr(), static_cast<int>(addr.sockAddrLen())) != 0) {
        m_errors.setError(::WSAGetLastError());
        ::closesocket(s);
        m_fd = -1;
        return false;
    }

    if(!m_localAddr.fromSockAddr(addr.sockAddr(), static_cast<int>(addr.sockAddrLen())))
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
        m_errors.setError(WSAEBADF);
        return false;
    }

    if (::listen(toWinSock(m_fd), backlog) != 0) {
        m_errors.setError(::WSAGetLastError());
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
    int len = sizeof(client_addr);

    // Windows completely lacks accept4(). Invoke classic accept and configuration pairs. ??? 
    SOCKET clientSock = ::accept(toWinSock(m_fd), reinterpret_cast<sockaddr *>(&client_addr), &len);

    if (clientSock == INVALID_SOCKET) {
        int err = ::WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
            m_errors.setError(err);
        return nullptr;
    }

    u_long nonBlocking = 1;
    ::ioctlsocket(clientSock, FIONBIO, &nonBlocking);

    JobIpAddr peerAddr;
    if(!peerAddr.fromSockAddr(reinterpret_cast<sockaddr *>(&client_addr), len))
        JOB_LOG_WARN("[TcpSocket] accept: Failed to parse peer address.");

    auto loop = m_loop.lock();
    if (!loop) {
        ::closesocket(clientSock);
        return nullptr;
    }

    auto sock = std::make_shared<TcpSocket>(loop, toJobFd(clientSock), peerAddr);
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

    int n = ::recv(toWinSock(m_fd), static_cast<char*>(buffer), static_cast<int>(size), 0);

    if (n < 0) {
        int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
            return 0;

        m_errors.setError(err);
        return -1;
    }
    if (n == 0) {
        disconnect();
        return 0;
    }
    return static_cast<ssize_t>(n);
}

ssize_t TcpSocket::write(const void *buffer, size_t size)
{
    if (m_fd < 0) return -1;

    std::lock_guard<std::mutex> lock(m_writeMutex);
    // Strip MSG_NOSIGNAL (Windows sockets do not drop signals on broken links)
    int n = ::send(toWinSock(m_fd), static_cast<const char*>(buffer), static_cast<int>(size), 0);

    if (n < 0) {
        int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return 0;
        m_errors.setError(err);
        return -1;
    }
    return static_cast<ssize_t>(n);
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
        ret = ::setsockopt(toWinSock(m_fd), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&val), sizeof(val));
        break;
    case SocketOption::KeepAlive:
        ret = ::setsockopt(toWinSock(m_fd), SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&val), sizeof(val));
        break;
    case SocketOption::TcpNoDelay:
        ret = ::setsockopt(toWinSock(m_fd), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&val), sizeof(val));
        break;
    case SocketOption::NonBlocking: {
        u_long mode = enable ? 1 : 0;
        ret = ::ioctlsocket(toWinSock(m_fd), FIONBIO, &mode);
        break;
    }
    default:
        break;
    }
    if (ret != 0) m_errors.setError(::WSAGetLastError());
}

bool TcpSocket::option(SocketOption option) const
{
    if (m_fd < 0) return false;
    int val = 0;
    int len = sizeof(val);

    switch (option) {
    case SocketOption::ReuseAddress:
        ::getsockopt(toWinSock(m_fd), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&val), &len);
        break;
    case SocketOption::KeepAlive:
        ::getsockopt(toWinSock(m_fd), SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&val), &len);
        break;
    case SocketOption::TcpNoDelay:
        ::getsockopt(toWinSock(m_fd), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&val), &len);
        break;
    case SocketOption::NonBlocking:
        // Win32 provides no method to query non-blocking state via getsockopt()
        JOB_LOG_WARN("[TcpSocket] Querying NonBlocking socket option is un-supported on Win32.");
        return false;
    default:
        return false;
    }
    return (val != 0);
}

std::string TcpSocket::peerAddress() const { return m_peerAddr.toString(false); }
uint16_t TcpSocket::peerPort() const { return m_peerAddr.port(); }
std::string TcpSocket::localAddress() const { return m_localAddr.toString(false); }

uint16_t TcpSocket::localPort() const
{
    if (m_localAddr.port() == 0 && m_fd != -1) {
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
    int len = sizeof(sa);
    if (m_fd != -1 && ::getsockname(toWinSock(m_fd), reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_localAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updateLocalInfo: Failed to parse local address.");
}

void TcpSocket::updatePeerInfo() {
    sockaddr_storage sa{};
    int len = sizeof(sa);
    if (m_fd != -1 && ::getpeername(toWinSock(m_fd), reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_peerAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updatePeerInfo: Failed to parse peer address.");
}

void TcpSocket::onEvents(threads::IOEvent events)
{
    if (threads::hasEvent(events, threads::IOEvent::Error) || threads::hasEvent(events, threads::IOEvent::HangUp)) {
        int error = 0;
        int len = sizeof(error);
        ::getsockopt(toWinSock(m_fd), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);
        m_errors.setError(error ? error : WSAEIO);
        if (onError)
            onError(error ? error : WSAEIO);
        disconnect();
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Write) && m_state.load() == SocketState::Connecting) {
        int error = 0;
        int len = sizeof(error);
        if (::getsockopt(toWinSock(m_fd), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) == 0 && error == 0) {
            m_state.store(SocketState::Connected);
            updateLocalInfo();
            updatePeerInfo();
            if (onConnect)
                onConnect();
        } else {
            m_errors.setError(error ? error : WSAEIO);
            if (onError)
                onError(error ? error : WSAEIO);
            disconnect();
        }
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Read)) {
        if (m_state.load() == SocketState::Connected){
            if (onRead)
                onRead(nullptr, 0);
        } else if (m_state.load() == SocketState::Listening) {
            if (onConnect)
                onConnect();
        }
    }
}

} // namespace job::net
