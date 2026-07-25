#include "tcp_server.h"
#include <job_logger.h>
namespace job::net {
TcpServer::TcpServer(threads::JobIoAsyncThread::Ptr loop, uint16_t port) :
    m_loop(std::move(loop)),
    m_port{port}
{
    m_listener = TcpSocket::create(m_loop);
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

    // Swap the list out under a brief lock rather than holding m_mutex while
    // calling disconnect() below disconnect() synchronously fires onDisconnect,
    // which reenters onClientDisconnect() and tries to lock m_mutex itself.
    // Holding the lock across that call would deadlock (std::mutex isn't
    // recursive) and mutate m_clients while this function's own loop was
    // iterating it. By the time any reentrant call happens here, m_clients is
    // already empty, so its erase() becomes a harmless no-op.
    std::vector<TcpClient::Ptr> clientsToStop;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clientsToStop.swap(m_clients);
    }

    for (auto &client : clientsToStop)
        client->disconnect();
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

        auto client = std::make_shared<TcpClient>(m_loop);
        client->setSocket(clientSock);

        // Weak captures: these lambdas are stored as members on `client` itself.
        // Capturing the shared_ptr by value here would make client hold a strong
        // reference to itself through its own callbacks — refcount never reaches
        // zero, every accepted connection leaks for the process lifetime.
        std::weak_ptr<TcpClient> weakClient = client;

        client->onMessage = [this, weakClient](const char *data, size_t len) {
            if (auto c = weakClient.lock(); c && onClientMessage)
                onClientMessage(c, data, len);
        };
        client->onDisconnect = [this, weakClient]() {
            if (auto c = weakClient.lock())
                onClientDisconnect(c);
        };
        client->onError = [this, weakClient](int err) {
            JOB_LOG_WARN("[TcpServer] Client disconnected with error: {}", err);
            if (auto c = weakClient.lock())
                onClientDisconnect(c);
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