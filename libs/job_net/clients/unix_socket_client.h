#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <job_io_async_thread.h>
#include "unix_socket.h"
#include "job_url.h"
#include "resolve/job_ipaddr.h"
#include "jobnet_export.h"
namespace job::net {
class JOBNET_EXPORT UnixClient : public std::enable_shared_from_this<UnixClient> {
public:
    using Ptr = std::shared_ptr<UnixClient>;

    struct PrivateTag {
    private:
        PrivateTag() = default;
        friend class UnixClient;
    };

    UnixClient(PrivateTag, threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size = 4096);

    [[nodiscard]] static Ptr create(threads::JobIoAsyncThread::Ptr loop, uint16_t buffer_size = 4096)
    {
        return std::make_shared<UnixClient>(PrivateTag{}, std::move(loop), buffer_size);
    }

    ~UnixClient();
    UnixClient(const UnixClient &) = delete;
    UnixClient &operator=(const UnixClient &) = delete;

    bool connectToHost(const JobIpAddr &ipaddr);
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