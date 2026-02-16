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
    using Ptr = std::shared_ptr<TcpServer>;

    explicit TcpServer(threads::JobIoAsyncThread::Ptr loop, uint16_t port = 0);
    ~TcpServer();

    TcpServer(const TcpServer &) = delete;
    TcpServer &operator=(const TcpServer &) = delete;

    bool start(const std::string &address, uint16_t port, int backlog = 5);
    void stop();

    [[nodiscard]] uint16_t port() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    std::function<void(TcpClient::Ptr)> onClientConnected;
    std::function<void(TcpClient::Ptr, const char*, size_t)> onClientMessage;
    std::function<void(TcpClient::Ptr)> onClientDisconnected;
    std::function<void(int)> onError;

private:
    void setupListenerCallbacks();
    void onClientConnect();
    void onClientDisconnect(TcpClient::Ptr client);

    threads::JobIoAsyncThread::Ptr m_loop;
    TcpSocket::Ptr m_listener;

    std::vector<TcpClient::Ptr> m_clients;
    std::mutex m_mutex;

    uint16_t m_port{0};
};

} // namespace job::net
// CHECKPOINT: v2.1
