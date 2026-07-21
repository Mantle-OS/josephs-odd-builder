#include "sockets/tcp_socket.h"
#include "win32/win32_socket_registry.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <algorithm>

#include <job_logger.h>
#include <job_io_async_thread.h>

namespace job::net {

// Fetch the centralized subsystem registry instance
extern win32::Win32SocketRegistry& getSocketRegistry();

TcpSocket::TcpSocket(threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

TcpSocket::TcpSocket(threads::JobIoAsyncThread::Ptr loop, int existing_token, const JobIpAddr &peerAddr) :
    ISocketIO(std::move(loop)),
    m_peerAddr(peerAddr)
{
    m_fd = existing_token;
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

    SOCKET nativeSocket = getSocketRegistry().lookup(m_fd);
    if (nativeSocket != INVALID_SOCKET)
        ::closesocket(nativeSocket);

    getSocketRegistry().release(m_fd);
    m_fd = -1;
}

void TcpSocket::disconnect()
{
    auto expected = SocketState::Connected;
    if (m_state.compare_exchange_strong(expected, SocketState::Closing) ||
        m_state.load() == SocketState::Listening) {
        closeSocket();
        if (onDisconnect)
            onDisconnect();
    } else {
        closeSocket();
    }
}

bool TcpSocket::connectToHost(const JobUrl &url)
{
    // High-level connection factory or JobResolver maps endpoints into JobIpAddr before execution.
    // If a raw URL fallback is forced, resolve via static infrastructure.
    JOB_LOG_WARN("[TcpSocket] Direct connectToHost called. Sockets should receive mapped JobIpAddr targets.");
    return false;
}

bool TcpSocket::connect(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    if (m_state.load() != SocketState::Unconnected) {
        if (m_state.load() == SocketState::Closed) {
            m_state.store(SocketState::Unconnected);
        } else {
            JOB_LOG_WARN("[TcpSocket] connect called in invalid state: {}", (int)m_state.load());
            return false;
        }
    }

    SOCKET s = ::socket(addr.family() == JobIpAddr::Family::IPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        m_errors.setError(::WSAGetLastError());
        return false;
    }

    // Allocate safe 32-bit registry token to prevent 64-bit pointer truncation
    m_fd = getSocketRegistry().allocate(s);
    if (m_fd < 0){
        ::closesocket(s);
        return false;
    }

    u_long nonBlocking = 1;
    ::ioctlsocket(s, FIONBIO, &nonBlocking);

    m_peerAddr = addr;

    if (::connect(s, addr.sockAddr(), static_cast<int>(addr.sockAddrLen())) == 0) {
        m_state.store(SocketState::Connected);
        registerEvents(
            threads::IOEvent::Read |
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered);
        updateLocalInfo();
        return true;
    }

    int lastError = ::WSAGetLastError();
    if (lastError == WSAEWOULDBLOCK) {
        m_state.store(SocketState::Connecting);
        registerEvents(
            threads::IOEvent::Write |
            threads::IOEvent::Read |
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered);
        return true;
    }

    closeSocket();
    m_errors.setError(lastError);
    return false;
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

    m_fd = getSocketRegistry().allocate(s);
    if (m_fd < 0) {
        ::closesocket(s);
        return false;
    }

    u_long nonBlocking = 1;
    ::ioctlsocket(s, FIONBIO, &nonBlocking);

    setOption(SocketOption::ReuseAddress, true);

    if (::bind(s, addr.sockAddr(), static_cast<int>(addr.sockAddrLen())) != 0) {
        m_errors.setError(::WSAGetLastError());
        closeSocket();
        return false;
    }

    updateLocalInfo();
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

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET || ::listen(s, backlog) != 0) {
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

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET)
        return nullptr;

    sockaddr_storage client_addr{};
    int len = sizeof(client_addr);

    SOCKET clientSock = ::accept(s, reinterpret_cast<sockaddr *>(&client_addr), &len);
    if (clientSock == INVALID_SOCKET) {
        int err = ::WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
            m_errors.setError(err);

        return nullptr;
    }

    u_long nonBlocking = 1;
    ::ioctlsocket(clientSock, FIONBIO, &nonBlocking);

    JobIpAddr peerAddr;
    if (!peerAddr.fromSockAddr(reinterpret_cast<sockaddr *>(&client_addr), len))
        JOB_LOG_WARN("[TcpSocket] accept: Failed to parse peer address.");

    auto loop = m_loop.lock();
    if (!loop) {
        ::closesocket(clientSock);
        return nullptr;
    }

    int clientToken = getSocketRegistry().allocate(clientSock);
    if (clientToken < 0) {
        ::closesocket(clientSock);
        return nullptr;
    }

    auto sock = std::make_shared<TcpSocket>(loop, clientToken, peerAddr);
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

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET)
        return -1;

    int n = ::recv(s, static_cast<char*>(buffer), static_cast<int>(size), 0);
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
    if (m_fd < 0)
        return -1;

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET)
        return -1;

    std::lock_guard<std::mutex> lock(m_writeMutex);
    int n = ::send(s, static_cast<const char*>(buffer), static_cast<int>(size), 0);
    if (n < 0) {
        int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return 0;
        }
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
    if (m_fd < 0)
        return;

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET)
        return;

    int ret = -1;
    int val = enable ? 1 : 0;

    switch (option) {
    case SocketOption::ReuseAddress:
        ret = ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&val), sizeof(val));
        break;
    case SocketOption::KeepAlive:
        ret = ::setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&val), sizeof(val));
        break;
    case SocketOption::TcpNoDelay:
        ret = ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&val), sizeof(val));
        break;
    case SocketOption::NonBlocking: {
        u_long mode = enable ? 1 : 0;
        ret = ::ioctlsocket(s, FIONBIO, &mode);
        break;
    }
    default:
        break;
    }
    if (ret != 0)
    {
        m_errors.setError(::WSAGetLastError());
    }
}

bool TcpSocket::option(SocketOption option) const
{
    if (m_fd < 0)
        return false;

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET)
        return false;

    int val = 0;
    int len = sizeof(val);

    switch (option) {
    case SocketOption::ReuseAddress:
        ::getsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&val), &len);
        break;
    case SocketOption::KeepAlive:
        ::getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&val), &len);
        break;
    case SocketOption::TcpNoDelay:
        ::getsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&val), &len);
        break;
    case SocketOption::NonBlocking:
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

uint16_t TcpSocket::localPort()
{
    if (m_localAddr.port() == 0 && m_fd != -1)
        updateLocalInfo();

    return m_localAddr.port();
}

void TcpSocket::dumpState() const
{
    JOB_LOG_DEBUG("[TcpSocket] token={} state={} peer={}:{} local={}:{}",
                  m_fd, (int)m_state.load(),
                  peerAddress(), peerPort(),
                  localAddress(), m_localAddr.port()
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
    if (m_fd == -1)
        return;

    SOCKET s = getSocketRegistry().lookup(m_fd);
    sockaddr_storage sa{};
    int len = sizeof(sa);
    if (s != INVALID_SOCKET && ::getsockname(s, reinterpret_cast<sockaddr*>(&sa), &len) == 0){
        if (!m_localAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[TcpSocket] updateLocalInfo: Failed to parse local address.");
    }
}

void TcpSocket::updatePeerInfo()
{
    if (m_fd == -1)
        return;

    SOCKET s = getSocketRegistry().lookup(m_fd);
    sockaddr_storage sa{};
    int len = sizeof(sa);
    if (s != INVALID_SOCKET && ::getpeername(s, reinterpret_cast<sockaddr*>(&sa), &len) == 0) {
        if (!m_peerAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len)) {
            JOB_LOG_WARN("[TcpSocket] updatePeerInfo: Failed to parse peer address.");
        }
    }
}

void TcpSocket::onEvents(threads::IOEvent events)
{
    if (m_fd < 0)
        return;

    SOCKET s = getSocketRegistry().lookup(m_fd);
    if (s == INVALID_SOCKET)
        return;

    if (threads::hasEvent(events, threads::IOEvent::Error) || threads::hasEvent(events, threads::IOEvent::HangUp)) {
        int error = 0;
        int len = sizeof(error);
        ::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);
        m_errors.setError(error ? error : WSAEIO);
        if (onError)
            onError(error ? error : WSAEIO);

        disconnect();
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Write) && m_state.load() == SocketState::Connecting) {
        int error = 0;
        int len = sizeof(error);
        if (::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) == 0 && error == 0) {
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
        const auto current_state = m_state.load();
        if (current_state == SocketState::Connected){

            if (onRead)
                onRead(nullptr, 0);

        } else if (current_state == SocketState::Listening) {
            if (onAccept){
                // Core multiplexer loop handles factory instantiations inside servers land via shared_ptr
                if (auto acceptedSocket = accept())
                    onAccept(acceptedSocket);
            }
        }
    }
}

} // namespace job::net