#pragma once

#include <cstdint>
#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

enum class JobTouchState : std::int32_t
{
    Began = 0,
    Updated,
    Ended,

    Unknown
};

class JobTouchEvent
{
public:
    constexpr JobTouchEvent() noexcept = default;

    constexpr JobTouchEvent(JobTimeValue time,
                            JobInputType type,
                            JobInputCode code,
                            JobInputValue value,
                            double x,
                            double y,
                            std::int32_t slot,
                            std::int32_t trackingId,
                            JobTouchState state) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_x(x),
        m_y(y),
        m_slot(slot),
        m_trackingId(trackingId),
        m_state(state)
    {
    }

    constexpr ~JobTouchEvent() = default;

    constexpr JobTouchEvent(const JobTouchEvent &) noexcept = default;
    constexpr JobTouchEvent &operator=(const JobTouchEvent &) noexcept = default;

    constexpr JobTouchEvent(JobTouchEvent &&) noexcept = default;
    constexpr JobTouchEvent &operator=(JobTouchEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr std::int32_t slot() const noexcept
    {
        return m_slot;
    }

    [[nodiscard]] constexpr std::int32_t trackingId() const noexcept
    {
        return m_trackingId;
    }

    [[nodiscard]] constexpr JobTouchState state() const noexcept
    {
        return m_state;
    }

private:
    JobTimeValue  m_time{};
    JobInputType  m_type{};
    JobInputCode  m_code{};
    JobInputValue m_value{};

    double        m_x{};
    double        m_y{};

    std::int32_t  m_slot{-1};
    std::int32_t  m_trackingId{-1};

    JobTouchState m_state{JobTouchState::Unknown};
};

static_assert(JobInputConcept<JobTouchEvent>);

} // namespace job::input