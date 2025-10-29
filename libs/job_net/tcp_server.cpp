#include "tcp_server.h"
#include <thread>
#include <algorithm>

namespace job::net {

TcpServer::TcpServer()
{
    m_loop = std::make_shared<threads::AsyncEventLoop>();
    m_listener = std::make_shared<TcpSocket>(m_loop);
}

TcpServer::TcpServer(const std::shared_ptr<threads::AsyncEventLoop> &loop)
    : m_loop(loop)
{
    m_listener = std::make_shared<TcpSocket>(m_loop);
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(const std::string &address, uint16_t port, int backlog)
{
    if (!m_listener->bind(address, port))
        return false;

    if (!m_listener->listen(backlog))
        return false;

    m_running.store(true);

    m_port = m_listener->localPort();

    std::thread(&TcpServer::acceptLoop, this).detach();
    return true;
}

void TcpServer::stop()
{
    m_running.store(false);
    m_listener->disconnect();
}

void TcpServer::acceptLoop()
{
    while (m_running.load(std::memory_order_relaxed)) {
        auto clientSock = m_listener->accept();
        if (!clientSock) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        auto client = std::make_shared<TcpClient>(m_loop);
        client->setSocket(clientSock);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_clients.push_back(client);
        }

        if (onClientConnected)
            onClientConnected(client);

        client->onMessage = [this, client](const char *data, size_t len) {
            if (onClientMessage)
                onClientMessage(client, data, len);
        };

        client->onDisconnect = [this, client]() {
            if (onClientDisconnected)
                onClientDisconnected(client);

            std::lock_guard<std::mutex> lock(m_mutex);
            m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), client),
                            m_clients.end());
        };

        client->registerEvents();
    }
}

uint16_t TcpServer::port() const noexcept
{
    return m_port;
}
bool TcpServer::isRunning() const noexcept
{
    return m_running.load();
}

} // namespace job::net
