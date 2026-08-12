#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <job_io_async_thread.h>

#include "job_url.h"
#include "jobnet_export.h"
#include "resolve/job_ipaddr.h"
#include "resolve/job_resolver.h"
#include "resolve/job_ssl_context.h"
#include "resolve/job_ssl_error.h"
#include "ssl_socket.h"
#include "tcp_socket.h"

namespace job::net {

class JOBNET_EXPORT SslClient {
public:
    using Ptr = std::shared_ptr<SslClient>;

    explicit SslClient(threads::JobIoAsyncThread::Ptr loop, JobSslContext::Ptr context, JobResolver::Ptr resolver = nullptr,
                       uint16_t bufferSize = 4096);

    SslClient(threads::JobIoAsyncThread::Ptr loop, SslSocket::Ptr socket, JobResolver::Ptr resolver = nullptr,
              uint16_t bufferSize = 4096);

    ~SslClient();

    SslClient(const SslClient &) = delete;
    SslClient &operator=(const SslClient &) = delete;
    SslClient(SslClient &&) = delete;
    SslClient &operator=(SslClient &&) = delete;

    [[nodiscard]] bool connectToHost(const JobIpAddr &ipaddr);
    [[nodiscard]] bool connectToHost(const JobUrl &url);

    void disconnect();

    [[nodiscard]] int64_t send(const void *data, size_t size);
    [[nodiscard]] int64_t send(const std::string &data);

    /*
     * SslClient is application-ready only after the TLS handshake completes.
     * Therefore isConnected() and isEncrypted() currently have the same
     * meaning.
     */
    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] bool isEncrypted() const noexcept;

    [[nodiscard]] JobSslError::SslErrNo lastError() const noexcept;
    [[nodiscard]] std::string lastErrorString() const;

    void setSocket(SslSocket::Ptr socket);
    void setResolver(JobResolver::Ptr resolver);
    void setContext(JobSslContext::Ptr context);

    [[nodiscard]] SslSocket::Ptr socket() const noexcept;
    [[nodiscard]] JobSslContext::Ptr context() const noexcept;

    std::function<void()>                     onConnect;
    std::function<void()>                     onEncrypted;
    std::function<void(const char *, size_t)> onMessage;
    std::function<void()>                     onDisconnect;
    std::function<void(int)>                  onSocketError;
    JobSslError::ErrorCallback                onSslError;

private:
    [[nodiscard]] bool createSocket();
    [[nodiscard]] bool prepareSocketForConnect();

    void applyResolverToTransport();
    void setupSocketCallbacks();
    void detachSocketCallbacks() noexcept;
    void closeSocket() noexcept;
    void readAvailableData();

    threads::JobIoAsyncThread::Ptr m_loop;
    JobSslContext::Ptr             m_context;
    JobResolver::Ptr               m_resolver;
    SslSocket::Ptr                 m_socket;
    std::vector<char>              m_readBuffer;
};

} // namespace job::net