#pragma once

#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

// Later this will well .....
// minimum()
// maximum()
// fuzz()
// flat()
// resolution()
// normalizedValue()
// etc
namespace job::input {

class JobGamepadAxisEvent
{
public:
    constexpr JobGamepadAxisEvent() noexcept = default;

    constexpr JobGamepadAxisEvent(JobTimeValue time,
                                  JobInputType type,
                                  JobInputCode code,
                                  JobInputValue value) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value)
    {
    }

    constexpr ~JobGamepadAxisEvent() = default;

    constexpr JobGamepadAxisEvent(const JobGamepadAxisEvent &) noexcept = default;
    constexpr JobGamepadAxisEvent &operator=(const JobGamepadAxisEvent &) noexcept = default;

    constexpr JobGamepadAxisEvent(JobGamepadAxisEvent &&) noexcept = default;
    constexpr JobGamepadAxisEvent &operator=(JobGamepadAxisEvent &&) noexcept = default;

    [[nodiscard]] constexpr JobTimeValue time() const noexcept
    {
        return m_time;
    }

    [[nodiscard]] constexpr JobInputType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr JobInputCode code() const noexcept
    {
        return m_code;
    }

    [[nodiscard]] constexpr JobInputValue value() const noexcept
    {
        return m_value;
    }

private:
    JobTimeValue  m_time{};
    JobInputType  m_type{};
    JobInputCode  m_code{};
    JobInputValue m_value{};
};

static_assert(JobInputConcept<JobGamepadAxisEvent>);

} // namespace job::input