#pragma once

#include <cstdint>
#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

enum class JobKeyState : std::int32_t
{
    Released = 0,
    Pressed  = 1,
    Repeated = 2,

    Unknown
};

class JobKeyEvent
{
public:
    constexpr JobKeyEvent() noexcept = default;

    constexpr JobKeyEvent(JobTimeValue time,
                          JobInputType type,
                          JobInputCode code,
                          JobInputValue value,
                          JobKeyState state) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_state(state)
    {
    }

    constexpr ~JobKeyEvent() = default;

    constexpr JobKeyEvent(const JobKeyEvent &) noexcept = default;
    constexpr JobKeyEvent &operator=(const JobKeyEvent &) noexcept = default;

    constexpr JobKeyEvent(JobKeyEvent &&) noexcept = default;
    constexpr JobKeyEvent &operator=(JobKeyEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr JobKeyState state() const noexcept
    {
        return m_state;
    }


private:
    JobTimeValue  m_time{};
    JobInputType  m_type{};
    JobInputCode  m_code{};
    JobInputValue m_value{};
    JobKeyState   m_state{JobKeyState::Unknown};
};

static_assert(JobInputConcept<JobKeyEvent>);

} // namespace job::input