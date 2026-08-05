#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <job_io_async_thread.h>
#include "udp_socket.h"
#include "job_url.h"
#include "resolve/job_ipaddr.h"
#include "resolve/job_resolver.h"
#include "jobnet_export.h"
namespace job::net {
class JOBNET_EXPORT UdpClient {
public:
    using Ptr = std::shared_ptr<UdpClient>;
    explicit UdpClient(threads::JobIoAsyncThread::Ptr loop, JobResolver::Ptr resolver = nullptr, uint16_t buffer_size = 4096);
    ~UdpClient();
    UdpClient(const UdpClient &) = delete;
    UdpClient &operator=(const UdpClient &) = delete;
    bool connectToHost(const JobIpAddr &ipaddr);
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
    void setResolver(JobResolver::Ptr resolver);
private:
    void setupSocketCallbacks();
    threads::JobIoAsyncThread::Ptr m_loop;
    JobResolver::Ptr m_resolver;
    UdpSocket::Ptr m_socket;
    std::atomic<bool> m_connected{false};
    std::vector<char> m_readBuffer;
};
} // namespace job::net