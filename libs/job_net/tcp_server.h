#pragma once

#include <atomic>
#include <memory>
#include <functional>

#include <async_event_loop.h>
#include "tcp_socket.h"

namespace job::net {

class TcpServer : public TcpSocket,
                  std::enable_shared_from_this<TcpServer>
{
public:

    using ServerPtr = std::shared_ptr<TcpServer>;

    explicit TcpServer();
    ~TcpServer();

    [[nodiscard]] bool start(const JobUrl &url, int backlog = 10) noexcept;
    [[nodiscard]] bool start(const JobIpAddr &addr, int backlog = 10) noexcept;
    [[nodiscard]] bool start(const std::string &address, uint16_t port, int backlog = 10) noexcept;


    void stop() noexcept;

    [[nodiscard]] bool isRunning() const noexcept
    {
        return m_running.load();
    }

    // Event callbacks
    std::function<void(ServerPtr)> onClientConnected;
    std::function<void(ServerPtr, const char *data, size_t len)> onClientData;
    std::function<void(ServerPtr)> onClientDisconnected;
    std::function<void(int)> onError;

    [[nodiscard]] uint16_t port() const noexcept;
    [[nodiscard]] std::string address() const noexcept;

private:
    void registerAcceptEvents();
    void handleAccept();
    void attachClientHandlers(ServerPtr client);

private:
    std::shared_ptr<threads::AsyncEventLoop> m_loop;
    TcpSocket m_listener;

    std::unordered_map<ServerPtr> m_clients;
    std::atomic<bool> m_running{false};

    std::string m_address;
    uint16_t m_port{0};
    std::shared_ptr<threads::AsyncEventLoop> m_loop;
};

} // namespace job::net
