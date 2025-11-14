#include "job_timer.h"
#include <utility>

namespace job::core {

JobTimer::JobTimer(uint64_t id, TimePoint next, Duration interval,
                   bool repeat, bool active,
                   std::function<void ()> callback) noexcept :
    m_id{id},
    m_nextFire{next},
    m_interval{interval},
    m_repeat{repeat},
    m_active{active},
    m_callback(std::move(callback))
{
    if (m_nextFire == TimePoint{})
        scheduleNext();
}

uint64_t JobTimer::id() const noexcept
{
    return m_id;
}

void JobTimer::set_id(uint64_t id)
{
    m_id = id;
}

bool JobTimer::repeat() const noexcept
{
    return m_repeat;
}

void JobTimer::set_repeat(bool repeat)
{
    m_repeat = repeat;
}

bool JobTimer::isActive() const noexcept
{
    return m_active;
}

void JobTimer::set_isActive(bool isActive){
    m_active = isActive;
}

void JobTimer::set_next(const TimePoint &next)
{
    m_nextFire = next;
}

void JobTimer::set_interval(const Duration &interval)
{
    m_interval = interval;
}

JobTimer::TimePoint JobTimer::next() const noexcept
{
    return m_nextFire;
}

JobTimer::Duration JobTimer::interval() const noexcept
{
    return m_interval;
}

bool JobTimer::expired(const TimePoint &right_now) const noexcept
{
    return m_active && right_now >= m_nextFire;
}

std::function<void ()> JobTimer::callback() const
{
    return m_callback;
}


void JobTimer::fire() noexcept
{
    if (!m_active || !m_callback)
        return;

    try {
        m_callback();
    } catch (...) {
        // swallow exceptions to keep timer loop alive
    }

    if (m_repeat)
        scheduleNext();
    else
        m_active = false;
}

void JobTimer::cancel() noexcept
{
    m_active = false;
}

void JobTimer::scheduleNext() noexcept
{
    m_nextFire = Clock::now() + m_interval;
}

void JobTimer::set_callback(const std::function<void ()> &callback)
{
    m_callback = callback;
}

} // job::core
// CHECKPOINT: v1
