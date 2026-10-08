#pragma once

#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

class JobDialEvent
{
public:
    constexpr JobDialEvent() noexcept = default;
    constexpr JobDialEvent(JobTimeValue time, JobInputType type, JobInputCode code, JobInputValue value, double delta) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_delta(delta)
    {
    }

    constexpr ~JobDialEvent() = default;

    constexpr JobDialEvent(const JobDialEvent &) noexcept = default;
    constexpr JobDialEvent &operator=(const JobDialEvent &) noexcept = default;

    constexpr JobDialEvent(JobDialEvent &&) noexcept = default;
    constexpr JobDialEvent &operator=(JobDialEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr double delta() const noexcept
    {
        return m_delta;
    }

private:
    JobTimeValue    m_time{};
    JobInputType    m_type{};
    JobInputCode    m_code{};
    JobInputValue   m_value{};
    double           m_delta{};
};

static_assert(JobInputConcept<JobDialEvent>);

} // namespace job::input