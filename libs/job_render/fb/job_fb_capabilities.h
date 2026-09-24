#pragma once

#include <cstdint>

#include <linux/fb.h>

namespace job::render {

class JobFbCapabilities
{
public:
    using Value = std::uint16_t;

    constexpr JobFbCapabilities() = default;

    explicit constexpr JobFbCapabilities(Value value) noexcept :
        m_value(value)
    {
    }

    ~JobFbCapabilities() = default;

    JobFbCapabilities(const JobFbCapabilities &) = default;
    JobFbCapabilities &operator=(const JobFbCapabilities &) = default;

    JobFbCapabilities(JobFbCapabilities &&) noexcept = default;
    JobFbCapabilities &operator=(JobFbCapabilities &&) noexcept = default;

    [[nodiscard]] constexpr Value value() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr bool has(Value capability) const noexcept
    {
        return (m_value & capability) == capability;
    }

    [[nodiscard]] constexpr bool fourCc() const noexcept
    {
        return has(FB_CAP_FOURCC);
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_value == 0;
    }

    [[nodiscard]] friend constexpr bool operator==(const JobFbCapabilities &, const JobFbCapabilities &) noexcept = default;

private:
    Value m_value{};
};

}