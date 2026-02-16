#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <job_io_async_thread.h>

#include "udp_socket.h"
#include "job_url.h"

namespace job::net {

class UdpClient {
public:
    using Ptr = std::shared_ptr<UdpClient>;
    explicit UdpClient(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size = 4096);
    ~UdpClient();

    UdpClient(const UdpClient &) = delete;
    UdpClient &operator=(const UdpClient &) = delete;

    bool connectToHost(const std::string &host, uint16_t port);
    bool connectToHost(const JobUrl &url);
    void disconnect();

    ssize_t send(const void *data, size_t size);
    ssize_t send(const std::string &data);

    ssize_t sendTo(const void *buffer, size_t size, const JobIpAddr &dest);

    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] SocketErrors::SocketErrNo lastError() const noexcept;
    [[nodiscard]] std::string lastErrorString() const noexcept;

    std::function<void()> onConnect;
    std::function<void(const char*, size_t)> onMessage;
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;

private:
    void setupSocketCallbacks();

    threads::JobIoAsyncThread::Ptr m_loop;
    UdpSocket::Ptr m_socket;
    std::atomic<bool> m_connected{false};

    std::vector<char> m_readBuffer;
};

} // namespace job::net
// CHECKPOINT: v1.1
