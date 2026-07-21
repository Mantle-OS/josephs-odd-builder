#include "unix_socket_server.h"

#include <job_logger.h>

namespace job::net {

UnixServer::UnixServer(std::shared_ptr<threads::JobIoAsyncThread> loop) :
    m_loop(std::move(loop))
{
    m_listener = std::make_shared<UnixSocket>(m_loop);
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

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& client : m_clients)
        client->disconnect();

    m_clients.clear();
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
        if (!clientSock) {
            JOB_LOG_WARN("[UnixServer] Accepted socket was not a UnixSocket!");
            return;
        }

        auto client = std::make_shared<UnixClient>(m_loop);
        client->setSocket(clientSock);
        std::weak_ptr<UnixClient> weakClient = client;
        client->onMessage = [this, weakClient](const char *data, size_t len) {
            auto c = weakClient.lock();
            if (!c) return;

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

void UnixServer::onClientConnect()
{
    while (true) {
        ISocketIO::Ptr clientSockBase = m_listener->accept();
        if (!clientSockBase)
            break;

        UnixSocket::Ptr clientSock = std::static_pointer_cast<UnixSocket>(clientSockBase);
        if (!clientSock) {
            JOB_LOG_WARN("[UnixServer] Accepted socket was not a UnixSocket!");
            continue;
        }

        auto client = std::make_shared<UnixClient>(m_loop);
        client->setSocket(clientSock);
        client->onMessage = [this, client](const char *data, size_t len) {
            if (onClientMessage)
                onClientMessage(client, data, len);
        };

        client->onDisconnect = [this, client]() {
            onClientDisconnect(client);
        };

        client->onError = [this, client](int err) {
            JOB_LOG_WARN("[UnixServer] Client disconnected with error: {}", err);
            onClientDisconnect(client);
        };

        if (onClientConnected)
            onClientConnected(client);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_clients.push_back(client);
        }
    }
}

void UnixServer::onClientDisconnect(UnixClient::Ptr client)
{
    if (onClientDisconnected)
        onClientDisconnected(client);

    std::lock_guard<std::mutex> lock(m_mutex);
    std::erase(m_clients, client);
}

} // namespace job::net
