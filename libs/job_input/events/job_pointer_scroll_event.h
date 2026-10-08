#pragma once

#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

class JobPointerScrollEvent
{
public:
    constexpr JobPointerScrollEvent() noexcept = default;
    constexpr JobPointerScrollEvent(JobTimeValue time,
                                    JobInputType type,
                                    JobInputCode code,
                                    JobInputValue value,
                                    double horizontal,
                                    double vertical ) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_horizontal(horizontal),
        m_vertical(vertical)
    {
    }

    constexpr ~JobPointerScrollEvent() = default;

    constexpr JobPointerScrollEvent(const JobPointerScrollEvent &) noexcept = default;
    constexpr JobPointerScrollEvent &operator=(const JobPointerScrollEvent &) noexcept = default;

    constexpr JobPointerScrollEvent(JobPointerScrollEvent &&) noexcept = default;
    constexpr JobPointerScrollEvent &operator=(JobPointerScrollEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr double horizontal() const noexcept
    {
        return m_horizontal;
    }

    [[nodiscard]] constexpr double vertical() const noexcept
    {
        return m_vertical;
    }

private:
    JobTimeValue    m_time{};
    JobInputType    m_type{};
    JobInputCode    m_code{};
    JobInputValue   m_value{};
    double          m_horizontal{};
    double          m_vertical{};
};

static_assert(JobInputConcept<JobPointerScrollEvent>);

} // namespace job::input