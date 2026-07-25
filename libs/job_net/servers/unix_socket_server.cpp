#include "unix_socket_server.h"

#include <job_logger.h>

namespace job::net {

UnixServer::UnixServer(threads::JobIoAsyncThread::Ptr loop) :
    m_loop(std::move(loop))
{
    m_listener = UnixSocket::create(m_loop);
}

UnixServer::~UnixServer()
{
    stop();
}

bool UnixServer::start(const std::string &path, int backlog)
{
    if (!m_listener) {
        JOB_LOG_ERROR("[UnixServer] Listener socket is null!");
        return false;
    }

    if (!m_listener->bind(path)) {
        JOB_LOG_ERROR("[UnixServer] Failed to bind to path: {}", path);
        return false;
    }

    // Store the path
    m_path = path;
    setupListenerCallbacks();

    if (!m_listener->listen(backlog)) {
        JOB_LOG_ERROR("[UnixServer] Failed to listen on path: {}", path);
        return false;
    }

    JOB_LOG_INFO("[UnixServer] Now listening on {}", m_path);
    return true;
}

void UnixServer::stop()
{
    if (m_listener)
        m_listener->disconnect();

    // Swap the list out under a brief lock rather than holding m_mutex while
    // calling disconnect() below — disconnect() synchronously fires onDisconnect,
    // which reenters onClientDisconnect() and tries to lock m_mutex itself.
    // Holding the lock across that call would deadlock (std::mutex isn't
    // recursive) and mutate m_clients while this function's own loop was
    // iterating it. By the time any reentrant call happens here, m_clients is
    // already empty, so its erase() becomes a harmless no-op. Same fix as
    // TcpServer::stop().
    std::vector<UnixClient::Ptr> clientsToStop;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clientsToStop.swap(m_clients);
    }

    for (auto &client : clientsToStop)
        client->disconnect();
}

std::string UnixServer::path() const noexcept
{
    return m_path;
}

bool UnixServer::isRunning() const noexcept
{
    return m_listener && m_listener->isOpen();
}

void UnixServer::setupListenerCallbacks()
{
    m_listener->onAccept = [this](std::shared_ptr<ISocketIO> acceptedSocket) {
        auto clientSock = std::static_pointer_cast<UnixSocket>(acceptedSocket);

        auto client = UnixClient::create(m_loop);
        client->setSocket(clientSock);

        // Weak captures: these lambdas are stored as members on `client` itself.
        // Capturing the shared_ptr by value here would make client hold a strong
        // reference to itself through its own callbacks — refcount never reaches
        // zero, every accepted connection leaks for the process lifetime. Same
        // fix as TcpServer::onClientConnect().
        std::weak_ptr<UnixClient> weakClient = client;

        client->onMessage = [this, weakClient](const char *data, size_t len) {
            auto c = weakClient.lock();
            if (!c)
                return;

            if (onClientMessage)
                onClientMessage(c, data, len);
        };

        client->onDisconnect = [this, weakClient]() {
            auto c = weakClient.lock();
            if (!c)
                return;

            JOB_LOG_INFO("[UnixServer] Client disconnecting");
            onClientDisconnect(c);
        };

        client->onError = [this, weakClient](int err) {
            auto c = weakClient.lock();
            if (!c)
                return;

            JOB_LOG_WARN("[UnixServer] Client error: {}", err);
            onClientDisconnect(c);
        };

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_clients.push_back(client);
        }

        if (onClientConnected)
            onClientConnected(client);
    };

    m_listener->onError = [this](int err) {
        JOB_LOG_ERROR("[UnixServer] Listener socket error: {}", err);
        if (onError)
            onError(err);
        stop();
    };
}

void UnixServer::onClientDisconnect(UnixClient::Ptr client)
{
    if (onClientDisconnected)
        onClientDisconnected(client);

    UnixClient::Ptr removed;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find(m_clients.begin(), m_clients.end(), client);
        if (it != m_clients.end()) {
            removed = *it;   // keep it alive past the erase
            m_clients.erase(it);
        }
    }

    // `removed` may be the last strong reference. If it is, ~UnixClient() (and
    // reentrantly, its socket's disconnect()) must not run synchronously here —
    // we're still nested inside that very socket's own onEvents() call further up
    // this stack (see conversation: the "pure virtual method called" crash). Posting
    // the actual drop lets that call finish and unwind cleanly first.
    if (removed && m_loop) {
        m_loop->post([kept = std::move(removed)]() mutable { kept.reset(); });
    }
}

} // namespace job::net