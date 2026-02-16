#include "tcp_server.h"

#include <job_logger.h>

namespace job::net {

TcpServer::TcpServer(std::shared_ptr<threads::JobIoAsyncThread> loop, uint16_t port) :
    m_loop(std::move(loop)),
    m_port{port}
{
    m_listener = std::make_shared<TcpSocket>(m_loop);
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(const std::string &address, uint16_t port, int backlog)
{
    if (!m_listener) {
        JOB_LOG_ERROR("[TcpServer] Listener socket is null!");
        return false;
    }

    if (!m_listener->bind(address, port)) {
        JOB_LOG_ERROR("[TcpServer] Failed to bind to {}:{}", address, port);
        return false;
    }

    // The Magic ....
    setupListenerCallbacks();

    if (!m_listener->listen(backlog)) {
        JOB_LOG_ERROR("[TcpServer] Failed to listen on {}:{}", address, port);
        return false;
    }

    m_port = m_listener->localPort();
    JOB_LOG_INFO("[TcpServer] Now listening on port {}", m_port);

    return true;
}

void TcpServer::stop()
{
    if (m_listener)
        m_listener->disconnect();

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& client : m_clients)
        client->disconnect();

    m_clients.clear();
}

uint16_t TcpServer::port() const noexcept
{
    return m_port;
}
bool TcpServer::isRunning() const noexcept
{
    return m_listener && m_listener->isOpen();
}

void TcpServer::setupListenerCallbacks()
{
    m_listener->onConnect = [this]() {
        onClientConnect();
    };

    m_listener->onError = [this](int err) {
        JOB_LOG_ERROR("[TcpServer] Listener socket error: {}", err);
        if (onError)
            onError(err);

        stop();
    };
}

void TcpServer::onClientConnect()
{
    while (true) {
        std::shared_ptr<ISocketIO> clientSockBase = m_listener->accept();
        if (!clientSockBase)
            break;

        TcpSocket::Ptr clientSock = std::static_pointer_cast<TcpSocket>(clientSockBase);
        if (!clientSock) {
            JOB_LOG_WARN("[TcpServer] Accepted socket was not a TcpSocket!");
            continue;
        }

        auto client = std::make_shared<TcpClient>(m_loop);
        client->setSocket(clientSock);

        client->onMessage = [this, client](const char *data, size_t len) {
            if (onClientMessage)
                onClientMessage(client, data, len);
        };

        client->onDisconnect = [this, client]() {
            onClientDisconnect(client);
        };

        client->onError = [this, client](int err) {
            JOB_LOG_WARN("[TcpServer] Client disconnected with error: {}", err);
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

void TcpServer::onClientDisconnect(TcpClient::Ptr client)
{
    if (onClientDisconnected)
        onClientDisconnected(client);

    std::lock_guard<std::mutex> lock(m_mutex);
    std::erase(m_clients, client);
}

} // namespace job::net
// CHECKPOINT: v2.1
