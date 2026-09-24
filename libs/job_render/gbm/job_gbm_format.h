#pragma once

#include <cstdint>
#include <memory>

#include <gbm.h>

#include "job_drm_format.h"
#include "job_render_format_type.h"

namespace job::render {

class JobGbmFormat
{
public:
    using Ptr  = std::shared_ptr<JobGbmFormat>;
    using WPtr = std::weak_ptr<JobGbmFormat>;
    using UPtr = std::unique_ptr<JobGbmFormat>;

    using Value = std::uint32_t;

    JobGbmFormat() = default;
    explicit constexpr JobGbmFormat(Value value) noexcept :
        m_value(value)
    {
    }

    explicit constexpr JobGbmFormat(JobDrmFormat format) noexcept :
        m_value(format.value())
    {
    }

    ~JobGbmFormat() = default;

    JobGbmFormat(const JobGbmFormat &) = default;
    JobGbmFormat &operator=(const JobGbmFormat &) = default;

    JobGbmFormat(JobGbmFormat &&) noexcept = default;
    JobGbmFormat &operator=(JobGbmFormat &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Value value)
    {
        return std::make_shared<JobGbmFormat>(value);
    }

    [[nodiscard]] static Ptr createShared(JobDrmFormat format)
    {
        return std::make_shared<JobGbmFormat>(format);
    }

    [[nodiscard]] static UPtr createUniq(Value value)
    {
        return std::make_unique<JobGbmFormat>(value);
    }

    [[nodiscard]] static UPtr createUniq(JobDrmFormat format)
    {
        return std::make_unique<JobGbmFormat>(format);
    }

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr JobDrmFormat drmFormat() const noexcept
    {
        return JobDrmFormat{m_value};
    }

    [[nodiscard]] constexpr FormatType renderFormat() const noexcept
    {
        return drmFormat().renderFormat();
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return drmFormat().valid();
    }

    [[nodiscard]] constexpr bool bigEndian() const noexcept
    {
        return drmFormat().bigEndian();
    }

    [[nodiscard]] friend constexpr bool operator==(const JobGbmFormat &, const JobGbmFormat &) = default;

private:
    Value m_value{DRM_FORMAT_INVALID};
};

}