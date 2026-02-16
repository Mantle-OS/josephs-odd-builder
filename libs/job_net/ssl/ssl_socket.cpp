#include "ssl_socket.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <job_logger.h>
#include "job_ssl_cert.h"

namespace job::net::ssl {

SSLSocket::SSLSocket(std::shared_ptr<threads::JobIoAsyncThread> loop):
    ISocketIO(std::move(loop))
{
    m_state.store(SocketState::Unconnected);
    // Create a default context. User can override via setCertificate()
    // Default handles system CA loading for verifying HuggingFace.
    m_certContext = std::make_shared<JobSslCert>();
}

SSLSocket::SSLSocket(threads::JobIoAsyncThread::Ptr loop, int existing_fd, const JobIpAddr &peerAddr):
    ISocketIO(std::move(loop)),
    m_peerAddr(peerAddr)
{
    m_fd = existing_fd;
    m_state.store(SocketState::Connected);

    m_certContext = std::make_shared<JobSslCert>();

    setOption(SocketOption::NonBlocking, true);
    updateLocalInfo();
}

SSLSocket::~SSLSocket()
{
    disconnect();
}

bool SSLSocket::connectToHost(const JobUrl &url)
{
    if (m_fd >= 0) disconnect();

    if (m_state.load() != SocketState::Unconnected) {
        // If state is Closed, reset to Unconnected to allow reuse
        if (m_state.load() == SocketState::Closed) {
            m_state.store(SocketState::Unconnected);
        } else {
            JOB_LOG_WARN("[SSLSocket] connectToHost called in invalid state: {}", (int)m_state.load());
            return false;
        }
    }

    if (!m_certContext || !m_certContext->isValid()) {
        JOB_LOG_ERROR("[SSLSocket] Invalid SSL Context");
        m_errors.recordError(SocketErrors::SocketErrNo::Invalid);
        return false;
    }

    m_targetHost = url.host();
    uint16_t port = url.port() ? url.port() : 443;

    // 1. Resolve DNS (Blocking for now, similar to TcpSocket implementation)
    struct addrinfo hints{}, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    int err = getaddrinfo(m_targetHost.c_str(), portStr.c_str(), &hints, &res);
    if (err != 0) {
        JOB_LOG_ERROR("[SSLSocket] DNS Error: {}", gai_strerror(err));
        m_errors.recordError(SocketErrors::SocketErrNo::DNSFailure);
        return false;
    }

    // 2. Create Socket & Connect
    bool connected = false;
    for (auto *p = res; p != nullptr; p = p->ai_next) {
        m_fd = ::socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK, p->ai_protocol);
        if (m_fd < 0)
            continue;


        // Update Peer info immediately
        if (!m_peerAddr.fromSockAddr(p->ai_addr, p->ai_addrlen))
            JOB_LOG_WARN("[SslSocket] connectToHost: Failed to parse peer address.");

        if (::connect(m_fd, p->ai_addr, p->ai_addrlen) == 0) {
            // Immediate TCP connection (rare for non-blocking)
            // Skip straight to Handshake
            setupSSL();
            m_state.store(SocketState::Connected);
            m_handshakeState = HandshakeState::Handshaking;
            registerEvents(EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET);
            connected = true;
            break;
        }

        if (errno == EINPROGRESS) {
            // Waiting for TCP
            m_state.store(SocketState::Connecting);
            m_handshakeState = HandshakeState::None; // Not ready for SSL yet
            registerEvents(EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
            connected = true;
            break;
        }

        ::close(m_fd);
        m_fd = -1;
    }

    freeaddrinfo(res);

    if (!connected) {
        m_errors.recordError(errno);
        return false;
    }

    return true;
}

// Called once TCP is connected
bool SSLSocket::setupSSL()
{
    if (m_ssl) SSL_free(m_ssl);

    m_ssl = SSL_new(m_certContext->nativeHandle());
    if (!m_ssl) {
        JOB_LOG_ERROR("[SSLSocket] Failed to create SSL handle");
        return false;
    }

    SSL_set_fd(m_ssl, m_fd);
    if (!m_targetHost.empty())
        SSL_set_tlsext_host_name(m_ssl, m_targetHost.c_str());

    // Set Non-Blocking logic for OpenSSL
    SSL_set_mode(m_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);
    SSL_set_mode(m_ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    return true;
}

void SSLSocket::performHandshake()
{
    int ret = SSL_connect(m_ssl); // Client Handshake

    if (ret == 1) {
        // Success!
        m_handshakeState = HandshakeState::Ready;
        m_state.store(SocketState::Connected);

        JOB_LOG_INFO("[SSLSocket] Secured connection to {} ({})", m_targetHost, SSL_get_cipher(m_ssl));

        // We are ready for data. Monitor Read.
        registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);

        if (onConnect) onConnect();
        return;
    }

    int err = SSL_get_error(m_ssl, ret);
    if (err == SSL_ERROR_WANT_READ) {
        // SSL needs data from server to continue
        registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
    } else if (err == SSL_ERROR_WANT_WRITE) {
        // SSL needs to send data to server
        registerEvents(EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET);
    }
    else {
        // Fatal
        char errBuf[256];
        ERR_error_string_n(ERR_get_error(), errBuf, sizeof(errBuf));
        JOB_LOG_ERROR("[SSLSocket] Handshake failed: {} (SSL code: {})", errBuf, err);
        m_sslErrors.recordError(err, m_ssl);
        disconnect();
    }
}


ssize_t SSLSocket::read(void *buffer, size_t size)
{
    if (m_state.load() != SocketState::Connected || !m_ssl)
        return -1;

    int n = SSL_read(m_ssl, buffer, static_cast<int>(size));

    if (n > 0)
        return n;

    int err = SSL_get_error(m_ssl, n);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        m_errors.recordError(EAGAIN); // Map to standard WouldBlock
        return -1;
    }
    if (err == SSL_ERROR_ZERO_RETURN) {
        disconnect();
        return 0; // EOF
    }

    m_sslErrors.recordError(err, m_ssl);
    return -1;
}

ssize_t SSLSocket::write(const void *buffer, size_t size)
{
    if (m_state.load() != SocketState::Connected || !m_ssl)
        return -1;

    std::lock_guard<std::mutex> lock(m_writeMutex);

    int n = SSL_write(m_ssl, buffer, static_cast<int>(size));

    if (n > 0)
        return n;

    int err = SSL_get_error(m_ssl, n);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        m_errors.recordError(EAGAIN);
        return -1;
    }

    m_sslErrors.recordError(err, m_ssl);
    return -1;
}

void SSLSocket::disconnect()
{
    if (m_state.load() == SocketState::Closed)
        return;

    if (m_ssl) {
        // Polite shutdown
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_fd >= 0) {
        if (auto loop = m_loop.lock()) loop->unregisterFD(m_fd);
        ::close(m_fd);
        m_fd = -1;
    }

    m_state.store(SocketState::Closed);
    m_handshakeState = HandshakeState::None;

    if (onDisconnect)
        onDisconnect();
}

void SSLSocket::closeSocket()
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

void SSLSocket::onEvents(uint32_t events)
{
    if (events & (EPOLLERR | EPOLLHUP)) {
        JOB_LOG_ERROR("[SSLSocket] Socket Error/HUP");
        m_errors.recordError(ECONNRESET);
        disconnect();
        if (onError) onError(ECONNRESET);
        return;
    }

    // Phase 1: TCP Connecting
    if (m_state.load() == SocketState::Connecting) {
        int err = 0;
        socklen_t len = sizeof(err);
        if (::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
            m_errors.recordError(err);
            if (onError) onError(err);
            disconnect();
            return;
        }

        // TCP Connected. Start SSL.
        setupSSL();
        m_handshakeState = HandshakeState::Handshaking;
        // Fallthrough to Handshake logic
    }

    // Phase 2: SSL Handshake
    if (m_handshakeState == HandshakeState::Handshaking) {
        performHandshake();
        return;
    }

    // Phase 3: Connected Data Transfer
    if (m_state.load() == SocketState::Connected) {
        if (events & EPOLLIN) {
            // Notify user data *might* be ready.
            // They call read(), which calls SSL_read().
            if (onRead)
                onRead(nullptr, 0);
        }

        // Note: We generally don't poll EPOLLOUT in Connected state
        // unless a write failed with WANT_WRITE previously.
        // For simplicity in this iteration, we assume write() drives the loop.
    }
}

//FIXME we might want a list of these ?
void SSLSocket::setCertificate(JobSslCert::Ptr certificate)
{
    if(m_certContext)
        m_certContext.reset();

    m_certContext = certificate;
}

std::string SSLSocket::peerAddress() const
{
    return m_peerAddr.toString(false);
}

uint16_t SSLSocket::peerPort() const
{
    return m_peerAddr.port();
}
std::string SSLSocket::localAddress() const
{
    return m_localAddr.toString(false);
}
uint16_t SSLSocket::localPort() const
{
    if (m_localAddr.port() == 0 && m_fd != -1) {
        // const_cast is ugly, but this is a good place to lazily update
        const_cast<SSLSocket*>(this)->updateLocalInfo();
    }
    return m_localAddr.port();


    return 0; /* TODO */
}

void SSLSocket::updateLocalInfo() {
    sockaddr_storage sa{};
    socklen_t len = sizeof(sa);
    if (m_fd != -1 && ::getsockname(m_fd, reinterpret_cast<sockaddr*>(&sa), &len) == 0)
        if(!m_localAddr.fromSockAddr(reinterpret_cast<sockaddr*>(&sa), len))
            JOB_LOG_WARN("[SslSocket] updateLocalInfo: Failed to parse local address.");

}

ISocketIO::SocketState SSLSocket::state() const noexcept
{
    return m_state.load();
}
SocketErrors::SocketErrNo SSLSocket::lastError() const noexcept
{
    return m_errors.lastError();
}

// FIXME
void SSLSocket::setOption(SocketOption option, bool enable)
{
    if (m_fd < 0)
        return;
    if (option == SocketOption::NonBlocking) {
        int flags = ::fcntl(m_fd, F_GETFL, 0);
        ::fcntl(m_fd, F_SETFL, enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
    }
    // TODO: Forward others to setsockopt
}

bool SSLSocket::option(SocketOption option) const
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

void SSLSocket::dumpState() const
{
    JOB_LOG_INFO("[SSLSocket] FD: {} Host: {} State: {}", m_fd, m_targetHost, (int)m_state.load());
}

bool SSLSocket::isEncrypted() const
{
    return m_state.load() == SocketState::Connected && m_ssl != nullptr;
}


bool SSLSocket::bind(const JobIpAddr &addr)
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

bool SSLSocket::bind(const std::string &address, uint16_t port)
{
    JobIpAddr addr(address, port);
    return bind(addr);
}


bool SSLSocket::listen(int)
{
    return false;
}
ISocketIO::Ptr SSLSocket::accept()
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

    auto sock = std::make_shared<SSLSocket>(loop, clientFd, peerAddr);
    sock->registerEvents(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);

    return sock;
}

} // namespace job::net::ssl
