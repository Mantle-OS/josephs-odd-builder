#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "sockets/tcp_socket.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <utility>

#include <job_logger.h>
#include <io/job_io_async_thread.h>

#include "win_fd_reg.h"

namespace job::net {

TcpSocket::TcpSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

TcpSocket::TcpSocket(
    PrivateTag,
    threads::JobIoAsyncThread::Ptr loop,
    int existing_fd,
    const JobIpAddr &peerAddr) :
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

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock != INVALID_SOCKET)
        ::closesocket(nativeSock);

    threads::WinFdReg::instance().release(m_fd);
    m_fd = -1;
}

void TcpSocket::disconnect()
{
    auto expected = SocketState::Connected;

    if (m_state.compare_exchange_strong(
            expected,
            SocketState::Closing) ||
        m_state.load() == SocketState::Listening) {

        closeSocket();

        if (onDisconnect)
            onDisconnect();

        return;
    }

    closeSocket();
}

bool TcpSocket::connectToHost(const JobIpAddr &ipaddr)
{
    if (m_fd >= 0)
        closeSocket();

    if (m_state.load() != SocketState::Unconnected) {
        if (m_state.load() == SocketState::Closed) {
            m_state.store(SocketState::Unconnected);
        } else {
            JOB_LOG_WARN(
                "[TcpSocket] connectToHost called in invalid state: {}",
                static_cast<int>(m_state.load())
                );
            return false;
        }
    }

    if (!ipaddr.isValid()) {
        JOB_LOG_WARN("[TcpSocket] connectToHost called with invalid JobIpAddr");
        m_errors.setError(WSAEINVAL);
        return false;
    }

    int addressFamily = AF_UNSPEC;

    switch (ipaddr.family()) {
    case JobIpAddr::Family::IPv4:
        addressFamily = AF_INET;
        break;
    case JobIpAddr::Family::IPv6:
        addressFamily = AF_INET6;
        break;
    default:
        // TcpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        JOB_LOG_WARN("[TcpSocket] connectToHost: unsupported address family");
        m_errors.setError(WSAEAFNOSUPPORT);
        return false;
    }

    const SOCKET nativeSock = ::socket(addressFamily, SOCK_STREAM, IPPROTO_TCP);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(::WSAGetLastError());
        return false;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(nativeSock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        m_errors.setError(::WSAGetLastError());
        ::closesocket(nativeSock);
        return false;
    }

    m_fd = threads::WinFdReg::instance().allocate(nativeSock);

    if (m_fd < 0) {
        ::closesocket(nativeSock);
        m_errors.setError(WSAEMFILE);
        return false;
    }

    m_peerAddr = ipaddr;
    m_state.store(SocketState::Connecting);

    const int connectResult = ::connect(
        nativeSock,
        ipaddr.sockAddr(),
        static_cast<int>(ipaddr.sockAddrLen())
        );

    if (connectResult == 0) {
        m_state.store(SocketState::Connected);
        updateLocalInfo();

        registerEvents(
            threads::IOEvent::Read |
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered
            );

        if (onConnect)
            onConnect();

        return true;
    }

    const int lastError = ::WSAGetLastError();

    if (lastError == WSAEWOULDBLOCK || lastError == WSAEINPROGRESS) {
        registerEvents(
            threads::IOEvent::Write |
            threads::IOEvent::Read |
            threads::IOEvent::Error |
            threads::IOEvent::HangUp |
            threads::IOEvent::EdgeTriggered
            );
        return true;
    }

    m_errors.setError(lastError);
    closeSocket();
    m_state.store(SocketState::Unconnected);
    return false;
}

bool TcpSocket::bind(const JobIpAddr &addr)
{
    if (m_fd >= 0)
        closeSocket();

    if (!addr.isValid()) {
        m_errors.setError(WSAEINVAL);
        return false;
    }

    int addressFamily = AF_UNSPEC;

    switch (addr.family()) {
    case JobIpAddr::Family::IPv4:
        addressFamily = AF_INET;
        break;

    case JobIpAddr::Family::IPv6:
        addressFamily = AF_INET6;
        break;

    default:
        // TcpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        m_errors.setError(WSAEAFNOSUPPORT);
        return false;
    }

    const SOCKET nativeSock = ::socket(addressFamily, SOCK_STREAM, IPPROTO_TCP);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(::WSAGetLastError());
        return false;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(
            nativeSock,
            FIONBIO,
            &nonBlocking) == SOCKET_ERROR) {

        m_errors.setError(::WSAGetLastError());
        ::closesocket(nativeSock);
        return false;
    }

    m_fd = threads::WinFdReg::instance().allocate(nativeSock);

    if (m_fd < 0) {
        ::closesocket(nativeSock);
        m_errors.setError(WSAEMFILE);
        return false;
    }

    setOption(SocketOption::ReuseAddress, true);

    if (::bind(
            nativeSock,
            addr.sockAddr(),
            static_cast<int>(addr.sockAddrLen())) == SOCKET_ERROR) {

        m_errors.setError(::WSAGetLastError());
        closeSocket();
        return false;
    }

    /*
     * Query the kernel instead of merely copying the requested address.
     * This captures an automatically selected ephemeral port when port
     * zero was supplied.
     */
    updateLocalInfo();

    return true;
}

bool TcpSocket::bind(const JobUrl &url)
{
    // TCP binds to a local interface, never a remote peer — a hostname needing DNS
    // resolution has no sensible meaning here, so this deliberately never touches the
    // resolver. Only accepts an already-numeric IPv4/IPv6 literal.
    const std::string &host = url.host();

    if (!JobIpAddr::isIPv4(host) && !JobIpAddr::isIPv6(host)) {
        JOB_LOG_ERROR("[TcpSocket] bind(url) requires a numeric IPv4/IPv6 host, got '{}' — "
                      "hostname resolution is not performed for bind()", host);
        m_errors.setError(WSAEINVAL);
        return false;
    }

    const JobIpAddr addr(host, url.port());

    if (!addr.isValid()) {
        JOB_LOG_ERROR("[TcpSocket] bind(url): failed to construct address from '{}'", host);
        m_errors.setError(WSAEINVAL);
        return false;
    }

    return bind(addr);
}

bool TcpSocket::listen(int backlog)
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
        threads::IOEvent::EdgeTriggered
        );

    updateLocalInfo();
    return true;
}

ISocketIO::Ptr TcpSocket::accept()
{
    if (m_fd < 0 ||
        m_state.load() != SocketState::Listening) {
        return nullptr;
    }

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return nullptr;

    sockaddr_storage clientAddress{};
    int addressLength = sizeof(clientAddress);

    const SOCKET clientSock = ::accept(
        nativeSock,
        reinterpret_cast<sockaddr *>(&clientAddress),
        &addressLength
        );

    if (clientSock == INVALID_SOCKET) {
        const int error = ::WSAGetLastError();

        if (error != WSAEWOULDBLOCK)
            m_errors.setError(error);

        return nullptr;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(
            clientSock,
            FIONBIO,
            &nonBlocking) == SOCKET_ERROR) {

        m_errors.setError(::WSAGetLastError());
        ::closesocket(clientSock);
        return nullptr;
    }

    const int clientFd =
        threads::WinFdReg::instance().allocate(clientSock);

    if (clientFd < 0) {
        ::closesocket(clientSock);
        m_errors.setError(WSAEMFILE);
        return nullptr;
    }

    JobIpAddr peerAddr;

    if (!peerAddr.fromSockAddr(
            reinterpret_cast<sockaddr *>(&clientAddress),
            static_cast<JobSockLen>(addressLength))) {
        JOB_LOG_WARN(
            "[TcpSocket] accept: Failed to parse peer address."
            );
    }

    auto loop = m_loop.lock();

    if (!loop) {
        threads::WinFdReg::instance().release(clientFd);
        ::closesocket(clientSock);
        return nullptr;
    }

    auto socket = TcpSocket::create(loop, clientFd, peerAddr);

    socket->registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered
        );

    return socket;
}

ssize_t TcpSocket::read(
    void *buffer,
    size_t size)
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

ssize_t TcpSocket::write( const void *buffer, size_t size)
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

    std::lock_guard<std::mutex> lock(m_writeMutex);

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

void TcpSocket::setOption(
    SocketOption option,
    bool enable)
{
    if (m_fd < 0)
        return;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return;

    int result = SOCKET_ERROR;
    const BOOL value = enable ? TRUE : FALSE;

    switch (option) {
    case SocketOption::ReuseAddress:
        result = ::setsockopt(
            nativeSock,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<const char *>(&value),
            sizeof(value)
            );
        break;

    case SocketOption::KeepAlive:
        result = ::setsockopt(
            nativeSock,
            SOL_SOCKET,
            SO_KEEPALIVE,
            reinterpret_cast<const char *>(&value),
            sizeof(value)
            );
        break;

    case SocketOption::TcpNoDelay:
        result = ::setsockopt(
            nativeSock,
            IPPROTO_TCP,
            TCP_NODELAY,
            reinterpret_cast<const char *>(&value),
            sizeof(value)
            );
        break;

    case SocketOption::NonBlocking: {
        u_long mode = enable ? 1UL : 0UL;

        result = ::ioctlsocket(
            nativeSock,
            FIONBIO,
            &mode
            );
        break;
    }

    default:
        return;
    }

    if (result == SOCKET_ERROR)
        m_errors.setError(::WSAGetLastError());
}

bool TcpSocket::option(SocketOption option) const
{
    if (m_fd < 0)
        return false;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return false;

    BOOL value = FALSE;
    int valueLength = sizeof(value);
    int result = SOCKET_ERROR;

    switch (option) {
    case SocketOption::ReuseAddress:
        result = ::getsockopt(
            nativeSock,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<char *>(&value),
            &valueLength
            );
        break;

    case SocketOption::KeepAlive:
        result = ::getsockopt(
            nativeSock,
            SOL_SOCKET,
            SO_KEEPALIVE,
            reinterpret_cast<char *>(&value),
            &valueLength
            );
        break;

    case SocketOption::TcpNoDelay:
        result = ::getsockopt(
            nativeSock,
            IPPROTO_TCP,
            TCP_NODELAY,
            reinterpret_cast<char *>(&value),
            &valueLength
            );
        break;


    // FIONBIO cannot be queried through getsockopt().
    case SocketOption::NonBlocking:
        return false;

    default:
        return false;
    }

    if (result == SOCKET_ERROR)
        return false;

    return value != FALSE;
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
    if (m_localAddr.port() == 0 && m_fd >= 0)
        const_cast<TcpSocket *>(this)->updateLocalInfo();

    return m_localAddr.port();
}

void TcpSocket::dumpState() const
{
    JOB_LOG_DEBUG(
        "[TcpSocket] fd={} state={} peer={}:{} local={}:{}",
        m_fd,
        static_cast<int>(m_state.load()),
        peerAddress(),
        peerPort(),
        localAddress(),
        localPort()
        );
}

bool TcpSocket::isOpen() const noexcept
{
    const auto currentState = m_state.load();

    return currentState == SocketState::Connected ||
           currentState == SocketState::Listening;
}

void TcpSocket::updateLocalInfo()
{
    if (m_fd < 0)
        return;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return;

    sockaddr_storage address{};
    int addressLength = sizeof(address);

    if (::getsockname(
            nativeSock,
            reinterpret_cast<sockaddr *>(&address),
            &addressLength) == SOCKET_ERROR) {
        return;
    }

    if (!m_localAddr.fromSockAddr(
            reinterpret_cast<sockaddr *>(&address),
            static_cast<JobSockLen>(addressLength))) {
        JOB_LOG_WARN(
            "[TcpSocket] updateLocalInfo: "
            "Failed to parse local address."
            );
    }
}

void TcpSocket::updatePeerInfo()
{
    if (m_fd < 0)
        return;

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET)
        return;

    sockaddr_storage address{};
    int addressLength = sizeof(address);

    if (::getpeername(
            nativeSock,
            reinterpret_cast<sockaddr *>(&address),
            &addressLength) == SOCKET_ERROR) {
        return;
    }

    if (!m_peerAddr.fromSockAddr(
            reinterpret_cast<sockaddr *>(&address),
            static_cast<JobSockLen>(addressLength))) {
        JOB_LOG_WARN(
            "[TcpSocket] updatePeerInfo: "
            "Failed to parse peer address."
            );
    }
}

void TcpSocket::onEvents(threads::IOEvent events)
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
            m_errors.setError(socketError);
            if (onError)
                onError(socketError);
        }
        // else: HangUp with no real SO_ERROR just means the peer closed normally —
        // not an error, don't report one. disconnect() below still fires onDisconnect.

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
            updatePeerInfo();

            registerEvents(
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

    if (threads::hasEvent(events, threads::IOEvent::Read)) {
        if (m_state.load() == SocketState::Connected) {
            if (onRead)
                onRead(nullptr, 0);

        } else if (m_state.load() == SocketState::Listening) {
            if (onConnect)
                onConnect();
        }
    }
}

} // namespace job::net