#pragma once

#include <memory>
#include <mutex>
#include <functional>
#include <vector>

#include <job_io_async_thread.h>

#include "unix_socket.h"
#include "clients/unix_socket_client.h"

namespace job::net {

class UnixServer {
public:
    using Ptr = std::shared_ptr<UnixServer>;
    explicit UnixServer(threads::JobIoAsyncThread::Ptr loop);
    ~UnixServer();

    UnixServer(const UnixServer &) = delete;
    UnixServer &operator=(const UnixServer &) = delete;

    bool start(const std::string &path, int backlog = 5);
    void stop();

    [[nodiscard]] std::string path() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    std::function<void(UnixClient::Ptr)> onClientConnected;
    std::function<void(UnixClient::Ptr, const char*, size_t)> onClientMessage;
    std::function<void(UnixClient::Ptr)> onClientDisconnected;
    std::function<void(int)> onError;

private:
    void setupListenerCallbacks();
    void onClientConnect();
    void onClientDisconnect(UnixClient::Ptr client);

    threads::JobIoAsyncThread::Ptr m_loop;
    UnixSocket::Ptr m_listener;

    std::vector<UnixClient::Ptr> m_clients;
    std::mutex m_mutex;

    std::string m_path;
};

} // namespace job::net
// CHECKPOINT: v1.1
