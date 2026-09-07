#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <job_io_async_thread.h>

#include "clients/ssl_client.h"
#include "clients/tcp_client.h"
#include "job_http_parser.h"
#include "job_http_request.h"
#include "job_http_response.h"
#include "jobnet_export.h"
#include "resolve/job_resolver.h"
#include "resolve/job_ssl_context.h"

namespace job::net {

class JOBNET_EXPORT JobHttpClient
{
public:
    using Ptr  = std::shared_ptr<JobHttpClient>;
    using WPtr = std::weak_ptr<JobHttpClient>;
    using UPtr = std::unique_ptr<JobHttpClient>;

    enum class State : std::uint8_t
    {
        Idle = 0,
        Connecting,
        Sending,
        Receiving,
        Complete,
        Error
    };

    enum class Transport : std::uint8_t
    {
        None = 0,
        Tcp,
        Ssl
    };

    using ResponseCallback      = std::function<void(const JobHttpResponse &)>;
    using ErrorCallback         = std::function<void(std::string_view)>;
    using ProgressCallback      = std::function<void(std::size_t received, std::size_t total, bool totalKnown)>;

    explicit JobHttpClient(threads::JobIoAsyncThread::Ptr loop,
                           JobResolver::Ptr resolver,
                           JobSslContext::Ptr sslContext);

    ~JobHttpClient();

    JobHttpClient(const JobHttpClient &) = delete;
    JobHttpClient &operator=(const JobHttpClient &) = delete;
    JobHttpClient(JobHttpClient &&) = delete;
    JobHttpClient &operator=(JobHttpClient &&) = delete;

    [[nodiscard]] static Ptr createShared(threads::JobIoAsyncThread::Ptr loop,
                                          JobResolver::Ptr resolver,
                                          JobSslContext::Ptr sslContext)
    {
        return std::make_shared<JobHttpClient>(std::move(loop), std::move(resolver), std::move(sslContext));
    }

    [[nodiscard]] static UPtr createUniq(threads::JobIoAsyncThread::Ptr loop,
                                         JobResolver::Ptr resolver,
                                         JobSslContext::Ptr sslContext)
    {
        return std::make_unique<JobHttpClient>(std::move(loop), std::move(resolver), std::move(sslContext));
    }

    [[nodiscard]] bool send(const JobHttpRequest &request);
    [[nodiscard]] bool send(JobHttpRequest &&request);

    void cancel();
    void reset();

    [[nodiscard]] State state() const noexcept;
    [[nodiscard]] Transport transport() const noexcept;

    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] bool isComplete() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;

    [[nodiscard]] const std::string &lastError() const noexcept;

    [[nodiscard]] const JobHttpRequest &request() const noexcept;
    [[nodiscard]] const JobHttpResponse &response() const noexcept;

    void setBodyMode(JobHttpParser::BodyMode mode) noexcept;
    [[nodiscard]] JobHttpParser::BodyMode bodyMode() const noexcept;

    void setBodyCallback(JobHttpParser::BodyCallback callback);
    void clearBodyCallback();

    void setResolver(JobResolver::Ptr resolver);
    [[nodiscard]] JobResolver::Ptr resolver() const noexcept;

    void setSslContext(JobSslContext::Ptr context);
    [[nodiscard]] JobSslContext::Ptr sslContext() const noexcept;

    ResponseCallback onResponse;
    ErrorCallback onError;
    ProgressCallback onProgress;

private:
    [[nodiscard]] bool beginRequest();
    [[nodiscard]] bool createTransport();
    [[nodiscard]] bool connectTransport();

    [[nodiscard]] bool buildRequestData();
    [[nodiscard]] bool sendPendingData();
    [[nodiscard]] bool setWriteInterest(bool enable);

    [[nodiscard]] NetIoResult sendTransport(const void *data, std::size_t size);

    void setupTcpCallbacks();
    void setupSslCallbacks();
    void detachTransportCallbacks() noexcept;
    void closeTransport() noexcept;

    void handleConnected();
    void handleWritable();
    void handleMessage(const char *data, std::size_t size);
    void handleDisconnected();
    void handleTransportError(std::string_view error);

    void completeResponse();
    void fail(std::string_view error);

private:
    threads::JobIoAsyncThread::Ptr m_loop;
    JobResolver::Ptr m_resolver;
    JobSslContext::Ptr m_sslContext;

    TcpClient::Ptr m_tcpClient;
    SslClient::Ptr m_sslClient;

    JobHttpRequest m_request;
    JobHttpParser m_parser;

    State m_state{State::Idle};
    Transport m_transport{Transport::None};

    std::string m_requestData;
    std::size_t m_requestBytesSent{0};

    std::string m_lastError;
};
} // namespace job::net