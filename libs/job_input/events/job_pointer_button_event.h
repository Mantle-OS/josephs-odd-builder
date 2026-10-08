#pragma once

#include <cstdint>
#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

enum class JobPointerButtonState : std::int32_t
{
    Released = 0,
    Pressed  = 1,

    Unknown
};

class JobPointerButtonEvent
{
public:
    constexpr JobPointerButtonEvent() noexcept = default;

    constexpr JobPointerButtonEvent(JobTimeValue time,
                                    JobInputType type,
                                    JobInputCode code,
                                    JobInputValue value,
                                    JobPointerButtonState state) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_state(state)
    {
    }

    constexpr ~JobPointerButtonEvent() = default;

    constexpr JobPointerButtonEvent(const JobPointerButtonEvent &) noexcept = default;
    constexpr JobPointerButtonEvent &operator=(const JobPointerButtonEvent &) noexcept = default;

    constexpr JobPointerButtonEvent(JobPointerButtonEvent &&) noexcept = default;
    constexpr JobPointerButtonEvent &operator=(JobPointerButtonEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr JobPointerButtonState state() const noexcept
    {
        return m_state;
    }

private:
    JobTimeValue          m_time{};
    JobInputType          m_type{};
    JobInputCode          m_code{};
    JobInputValue         m_value{};
    JobPointerButtonState m_state{JobPointerButtonState::Unknown};
};

static_assert(JobInputConcept<JobPointerButtonEvent>);

} // namespace job::input