#pragma once

#include "job_drm_format.h"
#include "job_drm_modifier.h"

namespace job::render {

class JobDrmFormatModifier
{
public:
    constexpr JobDrmFormatModifier() = default;

    constexpr JobDrmFormatModifier(JobDrmFormat format, JobDrmModifier modifier) noexcept :
        m_format(format),
        m_modifier(modifier)
    {
    }

    ~JobDrmFormatModifier() = default;

    JobDrmFormatModifier(const JobDrmFormatModifier &) = default;
    JobDrmFormatModifier &operator=(const JobDrmFormatModifier &) = default;

    JobDrmFormatModifier(JobDrmFormatModifier &&) noexcept = default;
    JobDrmFormatModifier &operator=(JobDrmFormatModifier &&) noexcept = default;

    [[nodiscard]] constexpr const JobDrmFormat &format() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] constexpr const JobDrmModifier &modifier() const noexcept
    {
        return m_modifier;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_format.valid() && m_modifier.valid();
    }

    [[nodiscard]] constexpr FormatType renderFormat() const noexcept
    {
        return m_format.renderFormat();
    }

    [[nodiscard]] constexpr bool linear() const noexcept
    {
        return m_modifier.linear();
    }

    [[nodiscard]] constexpr DrmModifierVendor vendor() const noexcept
    {
        return m_modifier.vendor();
    }

    [[nodiscard]] friend constexpr bool operator==(const JobDrmFormatModifier &, const JobDrmFormatModifier &) noexcept = default;

private:
    JobDrmFormat    m_format;
    JobDrmModifier  m_modifier;
};

}