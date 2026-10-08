#pragma once

#include <cstdint>
#include <utility>

#include "job_input_event_concept.h"
#include "job_input_traits.h"
#include "job_input_tv.h"

namespace job::input {

enum class JobTabletToolState : std::int32_t
{
    OutOfProximity = 0,
    InProximity    = 1,
    Unknown
};

class JobTabletToolEvent
{
public:
    constexpr JobTabletToolEvent() noexcept = default;

    constexpr JobTabletToolEvent(JobTimeValue time, JobInputType type, JobInputCode code, JobInputValue value,
                                 JobInputCode toolCode,
                                 double x, double y,
                                 double pressure,
                                 JobTabletToolState state) noexcept :
        m_time(std::move(time)),
        m_type(type),
        m_code(code),
        m_value(value),
        m_toolCode(toolCode),
        m_x(x),
        m_y(y),
        m_pressure(pressure),
        m_state(state)
    {
    }

    constexpr ~JobTabletToolEvent() = default;

    constexpr JobTabletToolEvent(const JobTabletToolEvent &) noexcept = default;
    constexpr JobTabletToolEvent &operator=(const JobTabletToolEvent &) noexcept = default;

    constexpr JobTabletToolEvent(JobTabletToolEvent &&) noexcept = default;
    constexpr JobTabletToolEvent &operator=(JobTabletToolEvent &&) noexcept = default;

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

    [[nodiscard]] constexpr JobInputCode toolCode() const noexcept
    {
        return m_toolCode;
    }

    [[nodiscard]] constexpr double x() const noexcept
    {
        return m_x;
    }

    [[nodiscard]] constexpr double y() const noexcept
    {
        return m_y;
    }

    [[nodiscard]] constexpr double pressure() const noexcept
    {
        return m_pressure;
    }

    [[nodiscard]] constexpr JobTabletToolState state() const noexcept
    {
        return m_state;
    }

private:
    JobTimeValue        m_time{};
    JobInputType        m_type{};
    JobInputCode        m_code{};
    JobInputValue       m_value{};
    JobInputCode        m_toolCode{};
    double              m_x{};
    double              m_y{};
    double              m_pressure{};
    JobTabletToolState  m_state{JobTabletToolState::Unknown};
};

static_assert(JobInputConcept<JobTabletToolEvent>);

} // namespace job::input