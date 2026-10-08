#pragma once

#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

class JobInputEvent
{
public:
    constexpr JobInputEvent() noexcept = default;
    constexpr JobInputEvent(JobTimeValue time, JobInputType type, JobInputCode code, JobInputValue value) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value)
    {
    }

    constexpr ~JobInputEvent() = default;

    constexpr JobInputEvent(const JobInputEvent &) noexcept = default;
    constexpr JobInputEvent &operator=(const JobInputEvent &) noexcept = default;

    constexpr JobInputEvent(JobInputEvent &&) noexcept = default;
    constexpr JobInputEvent &operator=(JobInputEvent &&) noexcept = default;

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
    JobTimeValue    m_time{};
    JobInputType    m_type{};
    JobInputCode    m_code{};
    JobInputValue   m_value{};
};

static_assert(JobInputConcept<JobInputEvent>);

} // namespace job::input