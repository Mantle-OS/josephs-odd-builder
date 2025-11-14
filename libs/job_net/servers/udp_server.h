#pragma once

#include <memory>

#include <functional>
#include <vector>

#include <job_io_async_thread.h>

#include "udp_socket.h"
#include "job_ipaddr.h"

namespace job::net {

class UdpServer {
public:
    using MessageCallback = std::function<void(const char*, size_t, const JobIpAddr&)>;

    explicit UdpServer(std::shared_ptr<threads::JobIoAsyncThread> loop, uint16_t buffer_size = 4096);
    ~UdpServer();

    UdpServer(const UdpServer &) = delete;
    UdpServer &operator=(const UdpServer &) = delete;

    bool start(const std::string &address, uint16_t port);
    bool start(const JobUrl &url);
    void stop();

    [[nodiscard]] uint16_t port() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    ssize_t sendTo(const void *buffer, size_t size, const JobIpAddr &dest);

    MessageCallback onMessage;
    std::function<void(int)> onError;

private:
    void setupSocketCallbacks();

    std::shared_ptr<threads::JobIoAsyncThread> m_loop;
    UdpSocket::UdpSocketPtr m_socket;

    uint16_t m_port{0};
    std::vector<char> m_readBuffer;
};

} // namespace job::net
// CHECKPOINT: v1.0
