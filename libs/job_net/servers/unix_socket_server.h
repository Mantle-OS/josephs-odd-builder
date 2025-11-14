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
    using ClientPtr = UnixClient::UnixClientPtr;

    explicit UnixServer(std::shared_ptr<threads::JobIoAsyncThread> loop);
    ~UnixServer();

    UnixServer(const UnixServer &) = delete;
    UnixServer &operator=(const UnixServer &) = delete;

    bool start(const std::string &path, int backlog = 5);
    void stop();

    [[nodiscard]] std::string path() const noexcept;
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
    UnixSocket::UnixSocketPtr m_listener;

    std::vector<ClientPtr> m_clients;
    std::mutex m_mutex;

    std::string m_path;
};

} // namespace job::net
// CHECKPOINT: v1.1
