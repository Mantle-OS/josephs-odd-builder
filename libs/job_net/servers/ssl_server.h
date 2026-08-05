#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <job_io_async_thread.h>

#include "clients/ssl_client.h"
#include "jobnet_export.h"
#include "resolve/job_ssl_context.h"
#include "resolve/job_ssl_error.h"
#include "tcp_socket.h"

namespace job::net {

class JOBNET_EXPORT SslServer {
public:
    using Ptr = std::shared_ptr<SslServer>;

    explicit SslServer(threads::JobIoAsyncThread::Ptr loop, JobSslContext::Ptr context, uint16_t port = 0,
                       uint16_t bufferSize = 4096);

    ~SslServer();

    SslServer(const SslServer &) = delete;
    SslServer &operator=(const SslServer &) = delete;
    SslServer(SslServer &&) = delete;
    SslServer &operator=(SslServer &&) = delete;

    [[nodiscard]] bool start(const std::string &address, uint16_t port, int backlog = 5);
    void stop();

    [[nodiscard]] uint16_t port() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] JobSslContext::Ptr context() const noexcept;

    void setContext(JobSslContext::Ptr context);

    std::function<void(SslClient::Ptr)>                       onClientConnected;
    std::function<void(SslClient::Ptr)>                       onClientEncrypted;
    std::function<void(SslClient::Ptr, const char *, size_t)> onClientMessage;
    std::function<void(SslClient::Ptr)>                       onClientDisconnected;
    std::function<void(int)>                                  onSocketError;
    std::function<void(SslClient::Ptr, JobSslError::SslErrNo, const std::string &)> onSslError;

private:
    [[nodiscard]] bool createListener();

    void setupListenerCallbacks();
    void detachListenerCallbacks() noexcept;
    void setupClientCallbacks(const SslClient::Ptr &client);
    void detachClientCallbacks(const SslClient::Ptr &client) noexcept;
    void acceptClients();
    void removeClient(const SslClient::Ptr &client);

    threads::JobIoAsyncThread::Ptr m_loop;
    JobSslContext::Ptr             m_context;
    TcpSocket::Ptr                 m_listener;
    std::vector<SslClient::Ptr>    m_clients;
    mutable std::mutex             m_mutex;
    uint16_t                       m_port{0};
    uint16_t                       m_bufferSize{4096};
};

} // namespace job::net