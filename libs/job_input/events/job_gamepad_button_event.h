#pragma once

#include <cstdint>
#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

enum class JobGamepadButtonState : std::int32_t
{
    Released = 0,
    Pressed  = 1,

    Unknown
};

class JobGamepadButtonEvent
{
public:
    constexpr JobGamepadButtonEvent() noexcept = default;

    constexpr JobGamepadButtonEvent(JobTimeValue time,
                                    JobInputType type,
                                    JobInputCode code,
                                    JobInputValue value,
                                    JobGamepadButtonState state) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_state(state)
    {
    }

    constexpr ~JobGamepadButtonEvent() = default;

    constexpr JobGamepadButtonEvent(const JobGamepadButtonEvent &) noexcept = default;
    constexpr JobGamepadButtonEvent &operator=(const JobGamepadButtonEvent &) noexcept = default;

    constexpr JobGamepadButtonEvent(JobGamepadButtonEvent &&) noexcept = default;
    constexpr JobGamepadButtonEvent &operator=(JobGamepadButtonEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr JobGamepadButtonState state() const noexcept
    {
        return m_state;
    }

private:
    JobTimeValue           m_time{};
    JobInputType           m_type{};
    JobInputCode           m_code{};
    JobInputValue          m_value{};
    JobGamepadButtonState  m_state{JobGamepadButtonState::Unknown};
};

static_assert(JobInputConcept<JobGamepadButtonEvent>);

} // namespace job::input