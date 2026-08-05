#include "ssl_client.h"

#include <algorithm>
#include <utility>

#include <job_logger.h>

namespace job::net {

SslClient::SslClient(threads::JobIoAsyncThread::Ptr loop, JobSslContext::Ptr context, JobResolver::Ptr resolver,
                     uint16_t bufferSize) :
    m_loop(std::move(loop)),
    m_context(std::move(context)),
    m_resolver(std::move(resolver)),
    m_readBuffer(std::max<size_t>(bufferSize, 1))
{
    if (!createSocket())
        JOB_LOG_ERROR("[SslClient] Failed to create SSL socket");
}

SslClient::SslClient(threads::JobIoAsyncThread::Ptr loop, SslSocket::Ptr socket, JobResolver::Ptr resolver,
                     uint16_t bufferSize) :
    m_loop(std::move(loop)),
    m_resolver(std::move(resolver)),
    m_readBuffer(std::max<size_t>(bufferSize, 1))
{
    setSocket(std::move(socket));
}

SslClient::~SslClient()
{
    detachSocketCallbacks();
    closeSocket();
}

bool SslClient::connectToHost(const JobIpAddr &ipaddr)
{
    if (!prepareSocketForConnect())
        return false;

    if (m_socket->connectToHost(ipaddr))
        return true;

    JOB_LOG_ERROR("[SslClient] Failed to connect to {}: {}", ipaddr.toString(true), m_socket->lastErrorString());
    return false;
}

bool SslClient::connectToHost(const JobUrl &url)
{
    if (!m_resolver) {
        JOB_LOG_ERROR("[SslClient] connectToHost(url) requires a resolver; call setResolver() first");

        if (onSocketError)
            onSocketError(static_cast<int>(SocketErrors::SocketErrNo::Invalid));

        return false;
    }

    if (!prepareSocketForConnect())
        return false;

    if (m_socket->connectToHost(url))
        return true;

    JOB_LOG_ERROR("[SslClient] Failed to connect to '{}': {}", url.toString(), m_socket->lastErrorString());
    return false;
}

void SslClient::disconnect()
{
    if (m_socket)
        m_socket->disconnect();
}

int64_t SslClient::send(const void *data, size_t size)
{
    if (!m_socket || !m_socket->isEncrypted())
        return -1;

    if (!data && size > 0)
        return -1;

    if (size == 0)
        return 0;

    return m_socket->write(data, size);
}

int64_t SslClient::send(const std::string &data)
{
    return send(data.data(), data.size());
}

bool SslClient::isConnected() const noexcept
{
    return m_socket && m_socket->state() == SslSocket::State::Encrypted;
}

bool SslClient::isEncrypted() const noexcept
{
    return m_socket && m_socket->isEncrypted();
}

JobSslError::SslErrNo SslClient::lastError() const noexcept
{
    return m_socket ? m_socket->lastError() : JobSslError::SslErrNo::None;
}

std::string SslClient::lastErrorString() const
{
    return m_socket ? m_socket->lastErrorString() : std::string{};
}

void SslClient::setSocket(SslSocket::Ptr socket)
{
    if (m_socket == socket)
        return;

    detachSocketCallbacks();
    closeSocket();

    m_socket = std::move(socket);

    if (!m_socket) {
        m_context.reset();
        return;
    }

    m_context = m_socket->context();
    applyResolverToTransport();
    setupSocketCallbacks();
}

void SslClient::setResolver(JobResolver::Ptr resolver)
{
    m_resolver = std::move(resolver);
    applyResolverToTransport();
}

void SslClient::setContext(JobSslContext::Ptr context)
{
    if (m_context == context)
        return;

    detachSocketCallbacks();
    closeSocket();

    m_socket.reset();
    m_context = std::move(context);

    if (m_context && !createSocket())
        JOB_LOG_ERROR("[SslClient] Failed to recreate SSL socket after context change");
}

SslSocket::Ptr SslClient::socket() const noexcept
{
    return m_socket;
}

JobSslContext::Ptr SslClient::context() const noexcept
{
    return m_context;
}

bool SslClient::createSocket()
{
    if (!m_loop) {
        JOB_LOG_ERROR("[SslClient] Cannot create socket without an I/O loop");
        return false;
    }

    if (!m_context || !m_context->isValid()) {
        JOB_LOG_ERROR("[SslClient] Cannot create socket without a valid SSL context");
        return false;
    }

    auto transport = TcpSocket::create(m_loop);

    if (!transport) {
        JOB_LOG_ERROR("[SslClient] Failed to create TCP transport");
        return false;
    }

    transport->setResolver(m_resolver);

    auto socket = SslSocket::create(std::move(transport), m_context);

    if (!socket || socket->state() == SslSocket::State::Error) {
        JOB_LOG_ERROR("[SslClient] Failed to create SSL socket");
        return false;
    }

    m_socket = std::move(socket);
    setupSocketCallbacks();

    return true;
}

bool SslClient::prepareSocketForConnect()
{
    if (!m_socket || m_socket->state() == SslSocket::State::Error) {
        detachSocketCallbacks();
        closeSocket();
        m_socket.reset();

        if (!createSocket()) {
            JOB_LOG_ERROR("[SslClient] Failed to create socket for connection");
            return false;
        }
    }

    return true;
}

void SslClient::applyResolverToTransport()
{
    if (!m_socket)
        return;

    const auto transport = std::dynamic_pointer_cast<TcpSocket>(m_socket->socket());

    if (transport)
        transport->setResolver(m_resolver);
}

void SslClient::setupSocketCallbacks()
{
    if (!m_socket)
        return;

    m_socket->onEncrypted = [this]() {
        /*
         * SslSocket begins its handshake when the TCP transport connects.
         * SslClient exposes onConnect as application-ready rather than as a
         * separate transport-level event.
         */
        if (onConnect)
            onConnect();

        if (onEncrypted)
            onEncrypted();
    };

    m_socket->onRead = [this](const char *, size_t) {
        readAvailableData();
    };

    m_socket->onDisconnect = [this]() {
        if (onDisconnect)
            onDisconnect();
    };

    m_socket->onSocketError = [this](int error) {
        if (onSocketError)
            onSocketError(error);
    };

    m_socket->onSslError = [this](JobSslError::SslErrNo error, const std::string &message) {
        /*
         * WANT_READ/WANT_WRITE and their connect/accept equivalents are
         * nonfatal state transitions. They may still be useful to callers,
         * so forward them without treating the client as disconnected.
         */
        if (JobSslError::isFatalSslError(error) && m_socket && m_socket->state() == SslSocket::State::Encrypted)
            JOB_LOG_ERROR("[SslClient] Fatal TLS error on an encrypted connection: {}", message);

        if (onSslError)
            onSslError(error, message);
    };
}

void SslClient::detachSocketCallbacks() noexcept
{
    if (!m_socket)
        return;

    m_socket->onEncrypted = nullptr;
    m_socket->onRead = nullptr;
    m_socket->onWrite = nullptr;
    m_socket->onDisconnect = nullptr;
    m_socket->onSocketError = nullptr;
    m_socket->onSslError = nullptr;
}

void SslClient::closeSocket() noexcept
{
    if (!m_socket)
        return;

    const SslSocket::State currentState = m_socket->state();

    if (currentState != SslSocket::State::Closed && currentState != SslSocket::State::ShuttingDown)
        m_socket->disconnect();
}

void SslClient::readAvailableData()
{
    const SslSocket::Ptr socket = m_socket;

    if (!socket)
        return;

    while (socket->isEncrypted()) {
        const int64_t count = socket->read(m_readBuffer.data(), m_readBuffer.size());

        if (count <= 0)
            break;

        if (onMessage)
            onMessage(m_readBuffer.data(), static_cast<size_t>(count));
    }
}

} // namespace job::net