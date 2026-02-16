#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <job_io_async_thread.h>

#include "unix_socket.h"
#include "job_url.h"

namespace job::net {

class UnixClient {
public:
    using Ptr = std::shared_ptr<UnixClient>;
    explicit UnixClient(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size = 4096);
    ~UnixClient();

    UnixClient(const UnixClient &) = delete;
    UnixClient &operator=(const UnixClient &) = delete;

    bool connectToHost(const std::string &path);
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

    void setSocket(UnixSocket::Ptr socket);
private:
    void setupSocketCallbacks();

    threads::JobIoAsyncThread::Ptr m_loop;
    UnixSocket::Ptr m_socket;
    std::atomic<bool> m_connected{false};
    std::vector<char> m_readBuffer;
};

} // namespace job::net
// CHECKPOINT: v1.2
