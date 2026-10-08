#pragma once

#include <cstdint>
#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

enum class JobSwitchState : std::int32_t
{
    Off = 0,
    On  = 1,

    Unknown
};

class JobSwitchEvent
{
public:
    constexpr JobSwitchEvent() noexcept = default;

    constexpr JobSwitchEvent(JobTimeValue time,
                             JobInputType type,
                             JobInputCode code,
                             JobInputValue value,
                             JobSwitchState state) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_state(state)
    {
    }

    constexpr ~JobSwitchEvent() = default;

    constexpr JobSwitchEvent(const JobSwitchEvent &) noexcept = default;
    constexpr JobSwitchEvent &operator=(const JobSwitchEvent &) noexcept = default;

    constexpr JobSwitchEvent(JobSwitchEvent &&) noexcept = default;
    constexpr JobSwitchEvent &operator=(JobSwitchEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr JobSwitchState state() const noexcept
    {
        return m_state;
    }

private:
    JobTimeValue  m_time{};
    JobInputType  m_type{};
    JobInputCode  m_code{};
    JobInputValue m_value{};

    JobSwitchState m_state{JobSwitchState::Unknown};
};

static_assert(JobInputConcept<JobSwitchEvent>);

} // namespace job::input