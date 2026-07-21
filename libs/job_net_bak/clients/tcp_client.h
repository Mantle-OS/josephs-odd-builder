#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <job_io_async_thread.h>

#include "tcp_socket.h"
#include "job_url.h"

namespace job::net {

class TcpClient {
public:
    using Ptr = std::shared_ptr<TcpClient>;
    explicit TcpClient(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size = 4096);
    ~TcpClient();

    TcpClient(const TcpClient &) = delete;
    TcpClient &operator=(const TcpClient &) = delete;

    bool connectToHost(const std::string &host, uint16_t port);
    bool connectToHost(const JobUrl &url);
    void disconnect();

    ssize_t send(const void *data, size_t size);
    ssize_t send(const std::string &data);

    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept;
    [[nodiscard]] std::string lastErrorString() const noexcept;

    std::function<void()> onConnect;
    std::function<void(const char*, size_t)> onMessage;
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;

    void setSocket(TcpSocket::Ptr socket);
private:
    void setupSocketCallbacks();

    threads::JobIoAsyncThread::Ptr m_loop;
    TcpSocket::Ptr m_socket;
    std::atomic<bool> m_connected{false};

    std::vector<char> m_readBuffer;
};

} // namespace job::net

