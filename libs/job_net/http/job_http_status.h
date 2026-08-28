#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "job_http_status_code.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobHttpStatus
{
public:
    using Ptr  = std::shared_ptr<JobHttpStatus>;
    using WPtr = std::weak_ptr<JobHttpStatus>;
    using UPtr = std::unique_ptr<JobHttpStatus>;

    JobHttpStatus() = default;
    explicit JobHttpStatus(std::uint16_t code) noexcept;
    explicit JobHttpStatus(JobHttpStatusCode status) noexcept;
    ~JobHttpStatus() = default;

    JobHttpStatus(const JobHttpStatus &) = default;
    JobHttpStatus &operator=(const JobHttpStatus &) = default;
    JobHttpStatus(JobHttpStatus &&) noexcept = default;
    JobHttpStatus &operator=(JobHttpStatus &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHttpStatus>();
    }

    [[nodiscard]] static Ptr createShared(std::uint16_t code)
    {
        return std::make_shared<JobHttpStatus>(code);
    }

    [[nodiscard]] static Ptr createShared(JobHttpStatusCode status)
    {
        return std::make_shared<JobHttpStatus>(status);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHttpStatus>();
    }

    [[nodiscard]] static UPtr createUniq(std::uint16_t code)
    {
        return std::make_unique<JobHttpStatus>(code);
    }

    [[nodiscard]] static UPtr createUniq(JobHttpStatusCode status)
    {
        return std::make_unique<JobHttpStatus>(status);
    }

    [[nodiscard]] std::uint16_t code() const noexcept;
    [[nodiscard]] JobHttpStatusCode status() const noexcept;
    [[nodiscard]] std::string_view statusString() const noexcept;

    void setCode(std::uint16_t code) noexcept;
    void setStatus(JobHttpStatusCode status) noexcept;

    [[nodiscard]] bool isKnown() const noexcept;
    [[nodiscard]] bool isInformational() const noexcept;
    [[nodiscard]] bool isSuccess() const noexcept;
    [[nodiscard]] bool isRedirection() const noexcept;
    [[nodiscard]] bool isClientError() const noexcept;
    [[nodiscard]] bool isServerError() const noexcept;

    [[nodiscard]] bool operator==(const JobHttpStatus &) const noexcept = default;
    [[nodiscard]] bool operator==(JobHttpStatusCode status) const noexcept;
    [[nodiscard]] bool operator==(std::uint16_t code) const noexcept;

private:
    std::uint16_t m_code{0};
};

} // namespace job::net