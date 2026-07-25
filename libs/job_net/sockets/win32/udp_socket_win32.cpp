#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "sockets/udp_socket.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

#include <job_logger.h>
#include <io/job_io_async_thread.h>

#include "win_fd_reg.h"

namespace job::net {

UdpSocket::UdpSocket(PrivateTag, threads::JobIoAsyncThread::Ptr loop) :
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
}

UdpSocket::~UdpSocket()
{
    closeSocket();
}

void UdpSocket::closeSocket()
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

void UdpSocket::updateLocalInfo()
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

    if (!m_boundAddr.fromSockAddr(
            reinterpret_cast<sockaddr *>(&address),
            static_cast<JobSockLen>(addressLength))) {
        JOB_LOG_WARN("[UdpSocket] updateLocalInfo: Failed to parse local address.");
    }
}

bool UdpSocket::connectToHost(const JobIpAddr &ipaddr)
{
    if (m_fd >= 0)
        closeSocket();

    if (!ipaddr.isValid()) {
        JOB_LOG_WARN("[UdpSocket] connectToHost called with invalid JobIpAddr");
        m_errors.setError(WSAEINVAL);
        m_state.store(SocketState::Error);
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
        // UdpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        JOB_LOG_WARN("[UdpSocket] connectToHost: unsupported address family");
        m_errors.setError(WSAEAFNOSUPPORT);
        m_state.store(SocketState::Error);
        return false;
    }

    // Reset state to Unconnected if we're reusing a closed socket
    m_state.store(SocketState::Unconnected);

    const SOCKET nativeSock = ::socket(addressFamily, SOCK_DGRAM, IPPROTO_UDP);

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

        m_errors.setError(::WSAGetLastError());
        closeSocket();
        return false;
    }

    m_peerAddr = ipaddr;
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

bool UdpSocket::bind(const JobIpAddr &addr)
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
        // UdpSocket only speaks IPv4/IPv6 — Unix-domain targets belong to UnixSocket.
        m_errors.setError(WSAEAFNOSUPPORT);
        return false;
    }

    // Reset state to Unconnected if we're reusing a closed socket
    m_state.store(SocketState::Unconnected);

    const SOCKET nativeSock = ::socket(addressFamily, SOCK_DGRAM, IPPROTO_UDP);

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

    setOption(SocketOption::ReuseAddress, true);

    if (::bind(
            nativeSock,
            addr.sockAddr(),
            static_cast<int>(addr.sockAddrLen())) == SOCKET_ERROR) {

        m_errors.setError(::WSAGetLastError());
        closeSocket();
        return false;
    }

    updateLocalInfo(); // Get kernel-assigned port
    // A bound-but-not-connect()'d UDP socket has no default peer — SocketState::Bound
    // is the correct state here, distinct from Connected. Same rule as the Linux side.
    m_state.store(SocketState::Bound);

    registerEvents(
        threads::IOEvent::Read |
        threads::IOEvent::Error |
        threads::IOEvent::HangUp |
        threads::IOEvent::EdgeTriggered
        );
    return true;
}

bool UdpSocket::bind(const JobUrl &url)
{
    // UDP binds to a local interface, never a remote peer — a hostname needing DNS
    // resolution has no sensible meaning here, so this deliberately never touches the
    // resolver. Only accepts an already-numeric IPv4/IPv6 literal. Same rule as
    // TcpSocket::bind(JobUrl).
    const std::string &host = url.host();

    if (!JobIpAddr::isIPv4(host) && !JobIpAddr::isIPv6(host)) {
        JOB_LOG_ERROR("[UdpSocket] bind(url) requires a numeric IPv4/IPv6 host, got '{}' — "
                      "hostname resolution is not performed for bind()", host);
        m_errors.setError(WSAEINVAL);
        return false;
    }

    const JobIpAddr addr(host, url.port());

    if (!addr.isValid()) {
        JOB_LOG_ERROR("[UdpSocket] bind(url): failed to construct address from '{}'", host);
        m_errors.setError(WSAEINVAL);
        return false;
    }

    return bind(addr);
}

bool UdpSocket::listen([[maybe_unused]] int backlog)
{
    // Not supported for UDP
    m_errors.setError(WSAEOPNOTSUPP);
    return false;
}

ISocketIO::Ptr UdpSocket::accept()
{
    // Not supported for UDP — connectionless, no per-client socket to hand off.
    m_errors.setError(WSAEOPNOTSUPP);
    return nullptr;
}

void UdpSocket::disconnect()
{
    if (m_fd >= 0)
        closeSocket();

    m_peerAddr.clear();
    m_state.store(SocketState::Closed); // Already handled by closeSocket, kept for parity with Linux

    if (onDisconnect)
        onDisconnect();
}

ssize_t UdpSocket::read(void *buffer, size_t size)
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
            return 0; // Not an error, just no data

        m_errors.setError(error);
        return -1;
    }

    return static_cast<ssize_t>(received);
}

ssize_t UdpSocket::write(const void *buffer, size_t size)
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
            return 0; // Not an error, buffer is full

        m_errors.setError(error);
        return -1;
    }

    return static_cast<ssize_t>(sent);
}

ssize_t UdpSocket::sendTo(const void *buffer, size_t size, const JobIpAddr &dest)
{
    if (m_fd < 0) {
        m_errors.setError(WSAENOTSOCK);
        return -1;
    }

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(WSAENOTSOCK);
        return -1;
    }

    const size_t boundedSize = std::min(
        size,
        static_cast<size_t>(std::numeric_limits<int>::max())
        );

    const int sent = ::sendto(
        nativeSock,
        static_cast<const char *>(buffer),
        static_cast<int>(boundedSize),
        0,
        dest.sockAddr(),
        static_cast<int>(dest.sockAddrLen())
        );

    if (sent == SOCKET_ERROR) {
        const int error = ::WSAGetLastError();

        if (error == WSAEWOULDBLOCK)
            return 0; // Not an error

        m_errors.setError(error);
        return -1;
    }

    return static_cast<ssize_t>(sent);
}

ssize_t UdpSocket::recvFrom(void *buffer, size_t size, JobIpAddr &sender)
{
    if (m_fd < 0) {
        m_errors.setError(WSAENOTSOCK);
        return -1;
    }

    const SOCKET nativeSock =
        threads::WinFdReg::instance().lookup(m_fd);

    if (nativeSock == INVALID_SOCKET) {
        m_errors.setError(WSAENOTSOCK);
        return -1;
    }

    const size_t boundedSize = std::min(
        size,
        static_cast<size_t>(std::numeric_limits<int>::max())
        );

    sockaddr_storage srcAddr{};
    int addressLength = sizeof(srcAddr);

    const int received = ::recvfrom(
        nativeSock,
        static_cast<char *>(buffer),
        static_cast<int>(boundedSize),
        0,
        reinterpret_cast<sockaddr *>(&srcAddr),
        &addressLength
        );

    if (received == SOCKET_ERROR) {
        const int error = ::WSAGetLastError();

        if (error == WSAEWOULDBLOCK)
            return 0; // Not an error

        m_errors.setError(error);
        return -1;
    }

    if (!sender.fromSockAddr(
            reinterpret_cast<sockaddr *>(&srcAddr),
            static_cast<JobSockLen>(addressLength))) {
        JOB_LOG_WARN("[UdpSocket] recvFrom: Failed to parse sender address.");
    }

    return static_cast<ssize_t>(received);
}

SocketErrors::SocketErrNo UdpSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

ISocketIO::SocketType UdpSocket::type() const noexcept
{
    return SocketType::Udp;
}

ISocketIO::SocketState UdpSocket::state() const noexcept
{
    return m_state.load();
}

void UdpSocket::setOption(SocketOption option, bool enable)
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

    case SocketOption::Broadcast:
        result = ::setsockopt(
            nativeSock,
            SOL_SOCKET,
            SO_BROADCAST,
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
        // Other TCP-specific options are ignored
        return;
    }

    if (result == SOCKET_ERROR)
        m_errors.setError(::WSAGetLastError());
}

bool UdpSocket::option(SocketOption option) const
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

    case SocketOption::Broadcast:
        result = ::getsockopt(
            nativeSock,
            SOL_SOCKET,
            SO_BROADCAST,
            reinterpret_cast<char *>(&value),
            &valueLength
            );
        break;

    /*
     * FIONBIO cannot be queried through getsockopt() — same limitation noted
     * in TcpSocket::option().
     */
    case SocketOption::NonBlocking:
        return false;

    default:
        return false;
    }

    if (result == SOCKET_ERROR)
        return false;

    return value != FALSE;
}

std::string UdpSocket::peerAddress() const
{
    return m_peerAddr.toString(false);
}

uint16_t UdpSocket::peerPort() const
{
    return m_peerAddr.port();
}

std::string UdpSocket::localAddress() const
{
    return m_boundAddr.toString(false);
}

uint16_t UdpSocket::localPort() const
{
    if (m_boundAddr.port() == 0 && m_fd >= 0)
        const_cast<UdpSocket *>(this)->updateLocalInfo();

    return m_boundAddr.port();
}

void UdpSocket::dumpState() const
{
    JOB_LOG_DEBUG(
        "[UdpSocket] fd={} state={} bound={}:{} peer={}:{}",
        m_fd,
        static_cast<int>(m_state.load()),
        localAddress(), localPort(),
        peerAddress(), peerPort()
        );
}

bool UdpSocket::isOpen() const noexcept
{
    const auto currentState = m_state.load();

    return currentState == SocketState::Connected ||
           currentState == SocketState::Listening ||
           currentState == SocketState::Bound;
}

void UdpSocket::onEvents(threads::IOEvent events)
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
        // else: HangUp with no real SO_ERROR isn't an error worth reporting.

        if (threads::hasEvent(events, threads::IOEvent::HangUp))
            disconnect(); // HUP is more serious

        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Read)) {
        if (onRead)
            onRead(nullptr, 0); // Signal that data is ready
    }
}

} // namespace job::net