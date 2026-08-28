#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "job_http_header.h"
#include "job_http_method.h"
#include "job_url.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobHttpRequest
{
public:
    using Ptr  = std::shared_ptr<JobHttpRequest>;
    using WPtr = std::weak_ptr<JobHttpRequest>;
    using UPtr = std::unique_ptr<JobHttpRequest>;

    using Body = std::vector<std::byte>;

    JobHttpRequest() = default;
    explicit JobHttpRequest(const JobUrl &url);
    JobHttpRequest(JobHttpMethod method, const JobUrl &url);
    JobHttpRequest(std::string_view customMethod, const JobUrl &url);
    ~JobHttpRequest() = default;

    JobHttpRequest(const JobHttpRequest &) = default;
    JobHttpRequest &operator=(const JobHttpRequest &) = default;
    JobHttpRequest(JobHttpRequest &&) = default;
    JobHttpRequest &operator=(JobHttpRequest &&) = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHttpRequest>();
    }

    [[nodiscard]] static Ptr createShared(const JobUrl &url)
    {
        return std::make_shared<JobHttpRequest>(url);
    }

    [[nodiscard]] static Ptr createShared(JobHttpMethod method, const JobUrl &url)
    {
        return std::make_shared<JobHttpRequest>(method, url);
    }

    [[nodiscard]] static Ptr createShared(std::string_view customMethod, const JobUrl &url)
    {
        return std::make_shared<JobHttpRequest>(customMethod, url);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHttpRequest>();
    }

    [[nodiscard]] static UPtr createUniq(const JobUrl &url)
    {
        return std::make_unique<JobHttpRequest>(url);
    }

    [[nodiscard]] static UPtr createUniq(JobHttpMethod method, const JobUrl &url)
    {
        return std::make_unique<JobHttpRequest>(method, url);
    }

    [[nodiscard]] static UPtr createUniq(std::string_view customMethod, const JobUrl &url)
    {
        return std::make_unique<JobHttpRequest>(customMethod, url);
    }

    [[nodiscard]] JobHttpMethod method() const noexcept;
    [[nodiscard]] std::string_view methodString() const noexcept;
    [[nodiscard]] const std::string &customMethod() const noexcept;

    void setMethod(JobHttpMethod method) noexcept;
    void setCustomMethod(std::string_view method);
    void clearCustomMethod() noexcept;

    [[nodiscard]] const JobUrl &url() const noexcept;
    void setUrl(const JobUrl &url);
    void setUrl(JobUrl &&url);

    [[nodiscard]] const JobHttpHeader &headers() const noexcept;
    [[nodiscard]] JobHttpHeader &headers() noexcept;

    void setHeaders(const JobHttpHeader &headers);
    void setHeaders(JobHttpHeader &&headers);
    void clearHeaders();

    [[nodiscard]] const Body &body() const noexcept;
    [[nodiscard]] std::span<const std::byte> bodyView() const noexcept;
    [[nodiscard]] bool hasBody() const noexcept;
    [[nodiscard]] std::size_t bodySize() const noexcept;

    void setBody(const Body &body);
    void setBody(Body &&body);
    void setBody(std::span<const std::byte> body);
    void clearBody() noexcept;

    void clear();

    [[nodiscard]] bool isValid() const noexcept;

private:
    JobHttpMethod m_method{JobHttpMethod::Get};
    std::string   m_customMethod;
    JobUrl        m_url;
    JobHttpHeader m_headers;
    Body          m_body;
};

} // namespace job::net