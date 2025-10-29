#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>

#include "tcp_socket.h"
#include "tcp_client.h"
namespace job::net {

class TcpServer {
public:
    using ClientPtr = std::shared_ptr<TcpClient>;

    explicit TcpServer();
    explicit TcpServer(const std::shared_ptr<threads::AsyncEventLoop> &loop);
    ~TcpServer();

    bool start(const std::string &address, uint16_t port, int backlog = 5);
    void stop();

    [[nodiscard]] uint16_t port() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    // Event callbacks
    std::function<void(ClientPtr)> onClientConnected;
    std::function<void(ClientPtr, const char*, size_t)> onClientMessage;
    std::function<void(ClientPtr)> onClientDisconnected;
    std::function<void(int)> onError;

private:
    void acceptLoop();

    std::shared_ptr<threads::AsyncEventLoop> m_loop;
    std::shared_ptr<TcpSocket> m_listener;
    std::vector<ClientPtr> m_clients;
    std::mutex m_mutex;
    std::atomic<bool> m_running{false};
    uint16_t m_port{0};
};

} // namespace job::net
