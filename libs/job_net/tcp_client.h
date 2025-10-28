#pragma once

#include <functional>
#include <atomic>
#include <string>

#include <job_timer.h>

#include <async_event_loop.h>

#include "tcp_socket.h"

namespace job::net {

class TcpClient final : public TcpSocket {
public:
    using ClientPtr = std::shared_ptr<TcpClient>;

    TcpClient() = default;
    TcpClient(const JobUrl &url, bool auto_connnect = false);
    TcpClient(const JobIpAddr &address, bool auto_connnect = false);
    ~TcpClient() override;

    [[nodiscard]] bool connectTo(const JobUrl &url, bool autoReconnect = false);
    [[nodiscard]] bool connectTo(const JobIpAddr &address, bool autoReconnect = false);

    void disconnect() noexcept override;

    [[nodiscard]] bool isConnected() const noexcept;

    // Lifecycle callbacks
    std::function<void()> onConnectSuccess;
    std::function<void(int)> onConnectError;
    std::function<void()> onDisconnectEvent;
    std::function<void(const char *data, size_t len)> onData;

    // Reconnect control
    void setReconnectDelay(core::JobTimer::Duration delay) noexcept;
    [[nodiscard]] core::JobTimer::Duration reconnectDelay() const noexcept;

private:
    void handleReconnect();
    void registerSocketEvents();

private:
    bool connectTo(const std::string &host, uint16_t port, bool autoReconnect = false);
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_autoReconnect{false};
    core::JobTimer::Duration m_reconnectDelay{2000};

    std::string m_host;
    uint16_t m_port{0};
};

} // namespace job::net
