#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "job_http_header.h"
#include "job_http_status.h"
#include "job_http_status_code.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobHttpResponse
{
public:
    using Ptr  = std::shared_ptr<JobHttpResponse>;
    using WPtr = std::weak_ptr<JobHttpResponse>;
    using UPtr = std::unique_ptr<JobHttpResponse>;

    using Body = std::vector<std::byte>;

    JobHttpResponse() = default;
    explicit JobHttpResponse(std::uint16_t code);
    explicit JobHttpResponse(JobHttpStatusCode statusCode);
    JobHttpResponse(std::uint16_t code, std::string_view reasonPhrase);
    JobHttpResponse(JobHttpStatusCode statusCode, std::string_view reasonPhrase);
    ~JobHttpResponse() = default;

    JobHttpResponse(const JobHttpResponse &) = default;
    JobHttpResponse &operator=(const JobHttpResponse &) = default;
    JobHttpResponse(JobHttpResponse &&) noexcept = default;
    JobHttpResponse &operator=(JobHttpResponse &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHttpResponse>();
    }

    [[nodiscard]] static Ptr createShared(std::uint16_t code)
    {
        return std::make_shared<JobHttpResponse>(code);
    }

    [[nodiscard]] static Ptr createShared(JobHttpStatusCode statusCode)
    {
        return std::make_shared<JobHttpResponse>(statusCode);
    }

    [[nodiscard]] static Ptr createShared(std::uint16_t code, std::string_view reasonPhrase)
    {
        return std::make_shared<JobHttpResponse>(code, reasonPhrase);
    }

    [[nodiscard]] static Ptr createShared(JobHttpStatusCode statusCode, std::string_view reasonPhrase)
    {
        return std::make_shared<JobHttpResponse>(statusCode, reasonPhrase);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHttpResponse>();
    }

    [[nodiscard]] static UPtr createUniq(std::uint16_t code)
    {
        return std::make_unique<JobHttpResponse>(code);
    }

    [[nodiscard]] static UPtr createUniq(JobHttpStatusCode statusCode)
    {
        return std::make_unique<JobHttpResponse>(statusCode);
    }

    [[nodiscard]] static UPtr createUniq(std::uint16_t code, std::string_view reasonPhrase)
    {
        return std::make_unique<JobHttpResponse>(code, reasonPhrase);
    }

    [[nodiscard]] static UPtr createUniq(JobHttpStatusCode statusCode, std::string_view reasonPhrase)
    {
        return std::make_unique<JobHttpResponse>(statusCode, reasonPhrase);
    }

    [[nodiscard]] const JobHttpStatus &status() const noexcept;
    [[nodiscard]] JobHttpStatus &status() noexcept;

    [[nodiscard]] std::uint16_t code() const noexcept;
    [[nodiscard]] JobHttpStatusCode statusCode() const noexcept;

    void setCode(std::uint16_t code) noexcept;
    void setStatusCode(JobHttpStatusCode statusCode) noexcept;

    [[nodiscard]] const std::string &reasonPhrase() const noexcept;
    void setReasonPhrase(std::string_view reasonPhrase);
    void clearReasonPhrase() noexcept;

    [[nodiscard]] const JobHttpHeader &headers() const noexcept;
    [[nodiscard]] JobHttpHeader &headers() noexcept;

    void setHeaders(const JobHttpHeader &headers);
    void setHeaders(JobHttpHeader &&headers);
    void clearHeaders();

    [[nodiscard]] const JobHttpHeader &trailers() const noexcept;
    [[nodiscard]] JobHttpHeader &trailers() noexcept;
    [[nodiscard]] bool hasTrailers() const noexcept;

    void setTrailers(const JobHttpHeader &trailers);
    void setTrailers(JobHttpHeader &&trailers);
    void clearTrailers();

    [[nodiscard]] const Body &body() const noexcept;
    [[nodiscard]] std::span<const std::byte> bodyView() const noexcept;
    [[nodiscard]] bool hasBody() const noexcept;
    [[nodiscard]] std::size_t bodySize() const noexcept;

    void setBody(const Body &body);
    void setBody(Body &&body);
    void setBody(std::span<const std::byte> body);
    void appendBody(std::span<const std::byte> data);
    void clearBody() noexcept;

    [[nodiscard]] bool isInformational() const noexcept;
    [[nodiscard]] bool isSuccessful() const noexcept;
    [[nodiscard]] bool isRedirect() const noexcept;
    [[nodiscard]] bool isClientError() const noexcept;
    [[nodiscard]] bool isServerError() const noexcept;

    void clear();

    [[nodiscard]] bool isValid() const noexcept;

private:
    JobHttpStatus m_status;
    std::string   m_reasonPhrase;
    JobHttpHeader m_headers;
    JobHttpHeader m_trailers;
    Body          m_body;
};

} // namespace job::net