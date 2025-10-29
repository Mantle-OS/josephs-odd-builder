#pragma once

#include "tcp_socket.h"
#include "job_url.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace job::net {

class TcpClient {
public:
    explicit TcpClient();
    explicit TcpClient(const std::shared_ptr<threads::AsyncEventLoop> &loop);
    ~TcpClient();

    // Connect / Disconnect
    bool connectToHost(const std::string &host, uint16_t port);
    bool connectToHost(const JobUrl &url);
    void disconnect();

    // Sending data
    ssize_t send(const void *data, size_t size);
    ssize_t send(const std::string &data);

    // Accessors
    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept;
    [[nodiscard]] std::string lastErrorString() const noexcept;



    // Event callbacks
    std::function<void()> onConnect;
    std::function<void(const char*, size_t)> onMessage;
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;

    void setSocket(const std::shared_ptr<ISocketIO> &socket);
    void registerEvents();



private:
    void setupSocketCallbacks();

    std::shared_ptr<threads::AsyncEventLoop> m_loop;
    std::shared_ptr<TcpSocket> m_socket;
    std::atomic<bool> m_connected{false};
};

} // namespace job::net
