#pragma once

#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

class JobPointerMotionEvent
{
public:
    constexpr JobPointerMotionEvent() noexcept = default;
    constexpr JobPointerMotionEvent(JobTimeValue time,
                                 JobInputType type,
                                 JobInputCode code,
                                 JobInputValue value,
                                 double x,
                                 double y,
                                 double deltaX,
                                 double deltaY) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_x(x),
        m_y(y),
        m_deltaX(deltaX),
        m_deltaY(deltaY)
    {
    }

    constexpr ~JobPointerMotionEvent() = default;

    constexpr JobPointerMotionEvent(const JobPointerMotionEvent &) noexcept = default;
    constexpr JobPointerMotionEvent &operator=(const JobPointerMotionEvent &) noexcept = default;

    constexpr JobPointerMotionEvent(JobPointerMotionEvent &&) noexcept = default;
    constexpr JobPointerMotionEvent &operator=(JobPointerMotionEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr double x() const noexcept
    {
        return m_x;
    }

    [[nodiscard]] constexpr double y() const noexcept
    {
        return m_y;
    }

    [[nodiscard]] constexpr double deltaX() const noexcept
    {
        return m_deltaX;
    }

    [[nodiscard]] constexpr double deltaY() const noexcept
    {
        return m_deltaY;
    }
private:
    JobTimeValue    m_time{};
    JobInputType    m_type{};
    JobInputCode    m_code{};
    JobInputValue   m_value{};
    double          m_x{};
    double          m_y{};
    double          m_deltaX{};
    double          m_deltaY{};
};

static_assert(JobInputConcept<JobPointerMotionEvent>);

} // namespace job::input