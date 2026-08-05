#include "ssl_server.h"

#include <algorithm>
#include <utility>

#include <job_logger.h>

namespace job::net {

SslServer::SslServer(threads::JobIoAsyncThread::Ptr loop, JobSslContext::Ptr context, uint16_t port, uint16_t bufferSize) :
    m_loop(std::move(loop)),
    m_context(std::move(context)),
    m_port(port),
    m_bufferSize(std::max<uint16_t>(bufferSize, 1))
{
    if (!m_loop) {
        JOB_LOG_ERROR("[SslServer] I/O loop is null");
        return;
    }

    if (!createListener())
        JOB_LOG_ERROR("[SslServer] Failed to create listener socket");
}

SslServer::~SslServer()
{
    stop();
}

bool SslServer::start(const std::string &address, uint16_t port, int backlog)
{
    if (!m_loop) {
        JOB_LOG_ERROR("[SslServer] Cannot start without an I/O loop");
        return false;
    }

    if (!m_context || !m_context->isValid()) {
        JOB_LOG_ERROR("[SslServer] Cannot start without a valid SSL context");
        return false;
    }

    if (isRunning()) {
        JOB_LOG_ERROR("[SslServer] Server is already running");
        return false;
    }

    if (!m_listener && !createListener())
        return false;

    if (!m_listener->bind(address, port)) {
        JOB_LOG_ERROR("[SslServer] Failed to bind to {}:{}: {}", address, port, m_listener->lastErrorString());
        return false;
    }

    setupListenerCallbacks();

    if (!m_listener->listen(backlog)) {
        JOB_LOG_ERROR("[SslServer] Failed to listen on {}:{}: {}", address, port, m_listener->lastErrorString());

        detachListenerCallbacks();
        m_listener->disconnect();
        return false;
    }

    m_port = m_listener->localPort();
#ifndef NDEBUG
    JOB_LOG_INFO("[SslServer] Now listening for TLS connections on port {}", m_port);
#endif
    return true;
}

void SslServer::stop()
{
    detachListenerCallbacks();

    if (m_listener)
        m_listener->disconnect();

    std::vector<SslClient::Ptr> clients;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clients.swap(m_clients);
    }

    /*
     * These callbacks capture this server. Remove them before initiating
     * shutdown so an asynchronous socket event cannot call a destroyed or
     * partially destroyed SslServer.
     */
    for (const auto &client : clients)
        detachClientCallbacks(client);

    /*
     * Do not hold m_mutex while disconnecting. The socket implementation may
     * synchronously complete part of its shutdown path.
     */
    for (const auto &client : clients) {
        if (client)
            client->disconnect();
    }
}

uint16_t SslServer::port() const noexcept
{
    return m_port;
}

bool SslServer::isRunning() const noexcept
{
    return m_listener && m_listener->isOpen();
}

JobSslContext::Ptr SslServer::context() const noexcept
{
    return m_context;
}

void SslServer::setContext(JobSslContext::Ptr context)
{
    if (m_context == context)
        return;

    if (isRunning()) {
        JOB_LOG_WARN("[SslServer] Cannot replace the SSL context while the server is running");
        return;
    }

    m_context = std::move(context);
}

bool SslServer::createListener()
{
    if (!m_loop) {
        JOB_LOG_ERROR("[SslServer] Cannot create a listener without an I/O loop");
        return false;
    }

    m_listener = TcpSocket::create(m_loop);

    if (!m_listener) {
        JOB_LOG_ERROR("[SslServer] Failed to create TCP listener");
        return false;
    }

    return true;
}

void SslServer::setupListenerCallbacks()
{
    if (!m_listener)
        return;

    m_listener->onConnect = [this]() {
        acceptClients();
    };

    m_listener->onError = [this](int error) {
        JOB_LOG_ERROR("[SslServer] Listener socket error: {}", error);

        if (onSocketError)
            onSocketError(error);

        stop();
    };
}

void SslServer::detachListenerCallbacks() noexcept
{
    if (!m_listener)
        return;

    m_listener->onConnect = nullptr;
    m_listener->onRead = nullptr;
    m_listener->onWrite = nullptr;
    m_listener->onDisconnect = nullptr;
    m_listener->onError = nullptr;
}

void SslServer::setupClientCallbacks(const SslClient::Ptr &client)
{
    if (!client)
        return;

    const std::weak_ptr<SslClient> weakClient = client;

    client->onEncrypted = [this, weakClient]() {
        const auto current = weakClient.lock();

        if (current && onClientEncrypted)
            onClientEncrypted(current);
    };

    client->onMessage = [this, weakClient](const char *data, size_t size) {
        const auto current = weakClient.lock();

        if (current && onClientMessage)
            onClientMessage(current, data, size);
    };

    client->onDisconnect = [this, weakClient]() {
        if (const auto current = weakClient.lock())
            removeClient(current);
    };

    client->onSocketError = [this, weakClient](int error) {
        const auto current = weakClient.lock();

        if (!current)
            return;

        JOB_LOG_WARN("[SslServer] Client socket error: {}", error);

        if (onSocketError)
            onSocketError(error);

        /*
         * Do not remove the client from inside the socket-error callback.
         * Request shutdown and let onDisconnect perform normal removal.
         */
        current->disconnect();
    };

    client->onSslError = [this, weakClient](JobSslError::SslErrNo error, const std::string &message) {
        const auto current = weakClient.lock();

        if (current && onSslError)
            onSslError(current, error, message);
    };
}

void SslServer::detachClientCallbacks(const SslClient::Ptr &client) noexcept
{
    if (!client)
        return;

    client->onConnect = nullptr;
    client->onEncrypted = nullptr;
    client->onMessage = nullptr;
    client->onDisconnect = nullptr;
    client->onSocketError = nullptr;
    client->onSslError = nullptr;
}

void SslServer::acceptClients()
{
    if (!m_listener || !m_context)
        return;

    while (true) {
        ISocketIO::Ptr accepted = m_listener->accept();

        if (!accepted)
            break;

        /*
         * A TcpSocket listener creates TcpSocket connections. This is an
         * invariant of TcpSocket::accept(), so no RTTI check is necessary.
         */
        auto transport = std::static_pointer_cast<TcpSocket>(std::move(accepted));
        auto sslSocket = SslSocket::create(transport, m_context);

        if (!sslSocket || sslSocket->state() == SslSocket::State::Error) {
            JOB_LOG_ERROR("[SslServer] Failed to wrap accepted TCP socket with TLS");
            transport->disconnect();
            continue;
        }

        auto client = std::make_shared<SslClient>(m_loop, sslSocket, nullptr, m_bufferSize);

        if (!client) {
            sslSocket->disconnect();
            continue;
        }

        setupClientCallbacks(client);

        /*
         * Retain the client before exposing it to application code. The
         * callback may synchronously reject and disconnect the connection.
         */
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_clients.push_back(client);
        }

        if (onClientConnected)
            onClientConnected(client);

        /*
         * onClientConnected may have rejected the connection. Only start TLS
         * while the accepted transport remains connected.
         */
        if (sslSocket->socketState() != ISocketIO::SocketState::Connected)
            continue;

        if (!sslSocket->startHandshake()) {
            JOB_LOG_ERROR("[SslServer] Failed to start client TLS handshake: {}", sslSocket->lastErrorString());
            client->disconnect();
        }
    }
}

void SslServer::removeClient(const SslClient::Ptr &client)
{
    if (!client)
        return;

    bool removed = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        const auto position = std::find(m_clients.begin(), m_clients.end(), client);

        if (position != m_clients.end()) {
            m_clients.erase(position);
            removed = true;
        }
    }

    if (!removed)
        return;

    detachClientCallbacks(client);

    if (onClientDisconnected)
        onClientDisconnected(client);
}

} // namespace job::net