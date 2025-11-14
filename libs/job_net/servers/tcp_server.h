#pragma once

#include <memory>
#include <mutex>
#include <functional>
#include <vector>

#include <job_io_async_thread.h>

#include "tcp_socket.h"
#include "clients/tcp_client.h"

namespace job::net {

class TcpServer {
public:
    using ClientPtr = std::shared_ptr<TcpClient>;

    explicit TcpServer(std::shared_ptr<threads::JobIoAsyncThread> loop, uint16_t port = 0);
    ~TcpServer();

    TcpServer(const TcpServer &) = delete;
    TcpServer &operator=(const TcpServer &) = delete;

    bool start(const std::string &address, uint16_t port, int backlog = 5);
    void stop();

    [[nodiscard]] uint16_t port() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    std::function<void(ClientPtr)> onClientConnected;
    std::function<void(ClientPtr, const char*, size_t)> onClientMessage;
    std::function<void(ClientPtr)> onClientDisconnected;
    std::function<void(int)> onError;

private:
    void setupListenerCallbacks();
    void onClientConnect();
    void onClientDisconnect(ClientPtr client);

    std::shared_ptr<threads::JobIoAsyncThread> m_loop;
    TcpSocketPtr m_listener;

    std::vector<ClientPtr> m_clients;
    std::mutex m_mutex;

    uint16_t m_port{0};
};

} // namespace job::net
// CHECKPOINT: v2.1
