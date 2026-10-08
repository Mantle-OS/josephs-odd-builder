#pragma once

#include <linux/input.h>

namespace job::input {

using JobEventSeconds      = decltype(input_event{}.input_event_sec);
using JobEventMicroseconds = decltype(input_event{}.input_event_usec);

class JobTimeValue
{
public:
    constexpr JobTimeValue() noexcept = default;

    constexpr JobTimeValue(JobEventSeconds sec, JobEventMicroseconds usec) noexcept :
        m_sec(sec),
        m_usec(usec)
    {
    }

    explicit constexpr JobTimeValue(const input_event &event) noexcept :
        m_sec(event.input_event_sec),
        m_usec(event.input_event_usec)
    {
    }

    constexpr ~JobTimeValue() = default;

    constexpr JobTimeValue(const JobTimeValue &) noexcept = default;
    constexpr JobTimeValue &operator=(const JobTimeValue &) noexcept = default;
    constexpr JobTimeValue(JobTimeValue &&) noexcept = default;
    constexpr JobTimeValue &operator=(JobTimeValue &&) noexcept = default;

    [[nodiscard]] constexpr JobEventSeconds sec() const noexcept
    {
        return m_sec;
    }

    [[nodiscard]] constexpr JobEventMicroseconds usec() const noexcept
    {
        return m_usec;
    }

private:
    JobEventSeconds m_sec{};
    JobEventMicroseconds m_usec{};
};

using JobEventTime = JobTimeValue;

} // namespace job::input