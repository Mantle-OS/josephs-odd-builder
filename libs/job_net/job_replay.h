#pragma once

#include <utility>
#include <string_view>
#include <vector>

#include "jobnet_export.h"
#include "sockets/job_socket_error.h"
#include "isocket_io.h"
#include "job_http_header.h"
#include "job_request.h"

// DONT USE YET NOT FINISHED YET

namespace job::net {
class JOBNET_EXPORT JobNetReply
{

public:
    explicit JobNetReply();
    ~JobNetReply();
    std::uint64_t readBufferSize() const;
    virtual void setReadBufferSize(std::uint64_t size);

    ISocketIO *socket() const;
    JobNetRequest request() const;
    SocketErrors::SocketErrNo error() const;
    bool isFinished() const;
    bool isRunning() const;
    JobUrl url() const;

    std::string header(JobIana::IanaHeaders header) const;

    bool hasRawHeader(std::string_view name) const;
    std::vector<std::string> rawHeaderList() const;
    std::string rawHeader(std::string_view headerName) const;

    const std::vector<std::pair<std::string, std::string>>  &rawHeaderPairs() const;
    JobHttpHeader headers() const;

    JobSslContext sslConfiguration() const;
    void setSslConfiguration(const JobSslContext &context);
    void ignoreSslErrors(const std::vector<JobSslError::SslErrNo> &errors);

    virtual void abort() = 0;
    virtual void ignoreSslErrors();
/*
    std::function<void> socketStartedConnecting();
    std::function<void requestSent();
    std::function<void metaDataChanged();
    std::function<void finished();
    std::function<void(SocketErrNo, const std::string&)> errorOccurred();

    void encrypted();
    void sslErrors(const std::vector<JobSslError> &errors);

    void redirected(const JobUrl &url);
    void redirectAllowed();

    void uploadProgress(std::uint64_t sent, std::uint64_t total);
    void downloadProgress(std::uint64_t received, std::uint64_t total);
*/
protected:
    virtual std::int64_t writeData(const char *data, std::size_t len);

    void setRequest(const JobNetRequest &request);
    void setError(SocketErrors::SocketErrNo errorno, const std::string &error);
    void setFinished(bool);
    void setUrl(const JobUrl &url);
    void setHeader(JobIana::IanaHeaders name, const std::string_view &value);
    void setRawHeader(const std::string_view &name, const std::string_view &value);
    void setHeaders(const JobHttpHeader &headers);
    void setHeaders(JobHttpHeader &&headers);

    virtual void sslContextImpl(JobSslContext &ctx) const;
    virtual void setSslContextImpl(const JobSslContext &ctx);
    virtual void ignoreSslErrorsImpl(const std::vector<JobSslError::SslErrNo> &);
};

}