#include "job_async_event_loop.h"

#include <job_logger.h>

namespace job::threads {

// I HATE THIS SO MUCH
constexpr size_t kDefaultTimerReservation = 32;
using namespace std::chrono_literals;
AsyncEventLoop::AsyncEventLoop()
{
    // FIXME
    // TODO revisit this(later) size if simulations show more need.
    m_timers.reserve(kDefaultTimerReservation);
}

AsyncEventLoop::~AsyncEventLoop() noexcept
{
    stop();
}

void AsyncEventLoop::start(bool realTime)
{
    if (m_running.exchange(true))
        return;

    JobThreadOptions opts = realTime ?
                                JobThreadOptions::realtimeDefault() :
                                JobThreadOptions::normal();
    std::snprintf(opts.name.data(), opts.name.size(), "AsyncLoop");

    m_thread = std::make_shared<JobThread>(opts);

    auto idle_heartbeat = std::chrono::milliseconds(opts.heartbeat);
    m_thread->setRunFunction([this, idle_heartbeat](std::stop_token token) {
        this->loop(token, idle_heartbeat);
    });

    if (m_thread->start() != JobThread::StartResult::Started) {
        m_running.store(false);
        JOB_LOG_ERROR("[AsyncEventLoop] Failed to start thread This is not good");
    }
}

void AsyncEventLoop::stop()
{
    if (!m_running.exchange(false))
        return;

    if (m_thread)
        m_thread->requestStop();

    {
        std::scoped_lock lock(m_timerMutex);
        m_timers.clear();
    }

    m_queue.stop();

    if (m_thread) {
        (void)m_thread->join();
        m_thread.reset();
    }
}

bool AsyncEventLoop::isRunning() const
{
    return m_running.load(std::memory_order_relaxed);
}

void AsyncEventLoop::post(std::function<void()> task, int priority)
{
    m_queue.post(std::move(task), priority);
}

uint64_t AsyncEventLoop::postDelayed(std::function<void()> task, std::chrono::milliseconds delay)
{
    return addTimer(std::move(task), delay, false);
}

uint64_t AsyncEventLoop::addTimer(std::function<void()> callback,
                                  std::chrono::milliseconds interval,
                                  bool repeat)
{
    const uint64_t id = m_nextTimerId.fetch_add(1, std::memory_order_relaxed);
    const auto now = std::chrono::steady_clock::now();

    core::JobTimer timer;
    timer.set_id(id);
    timer.set_next(now + interval);
    timer.set_interval(interval);
    timer.set_repeat(repeat);
    timer.set_isActive(true);
    timer.set_callback(std::move(callback));
    {
        std::scoped_lock lock(m_timerMutex);
        m_timers.push_back(std::move(timer));
    }

    // Wake up sleepy head
    m_queue.post([] {}, 0);
    return id;
}


bool AsyncEventLoop::cancelTimer(uint64_t id)
{
    std::scoped_lock lock(m_timerMutex);
    const auto before = m_timers.size();
    std::erase_if(m_timers, [id](const core::JobTimer &t) {
        return t.id() == id;
    });

    if (m_timers.size() != before) {
        post([] {}, 0);
    }
    return m_timers.size() != before;
}

std::chrono::milliseconds AsyncEventLoop::calculateNextWakeup() const
{
    std::scoped_lock lock(m_timerMutex);

    if (m_timers.empty())
        return std::chrono::milliseconds::max();

    const auto now = std::chrono::steady_clock::now();
    core::JobTimer::TimePoint soonest_time = std::chrono::steady_clock::time_point::max();
    bool foundActive = false;

    for (const auto& timer : m_timers) {
        if (timer.isActive()) {
            if (!foundActive || timer.next() < soonest_time) {
                soonest_time = timer.next();
                foundActive = true;
            }
        }
    }

    if (!foundActive)
        return std::chrono::milliseconds::max();

    if (soonest_time <= now)
        return std::chrono::milliseconds(0);

    return std::chrono::duration_cast<std::chrono::milliseconds>(soonest_time - now);
}

void AsyncEventLoop::processTimers()
{
    const auto now = std::chrono::steady_clock::now();

    std::scoped_lock lock(m_timerMutex);

    for (auto &timer : m_timers) {
        if (timer.expired(now)) {
            post(timer.callback(), 0);
            if (timer.repeat()) {
                timer.set_next(now + timer.interval());
            } else {
                timer.set_isActive(false);
            }
        }
    }

    std::erase_if(m_timers, [](const core::JobTimer &t) {
        return !t.isActive();
    });
}
void AsyncEventLoop::processTasks()
{
    for (int i = 0; i < 100; ++i) {
        auto taskOpt = m_queue.take(0ms); // Non-blocking take
        if (taskOpt.has_value()) {
            if (*taskOpt) {
                try {
                    (*taskOpt)();
                } catch (const std::exception& e) {
                    JOB_LOG_ERROR("[AsyncEventLoop] Task exception: %s", e.what());
                } catch (...) {
                    JOB_LOG_ERROR("[AsyncEventLoop] Task unknown exception");
                }
            }
        } else {
            break;
        }
    }
}

void AsyncEventLoop::loop(std::stop_token token, std::chrono::milliseconds idle_heartbeat)
{
    while (!token.stop_requested()) {

        auto next_wakeup = calculateNextWakeup();
        if (next_wakeup == std::chrono::milliseconds::max() && m_queue.isEmpty())
            next_wakeup = idle_heartbeat;
        else if (next_wakeup == std::chrono::milliseconds::max() && !m_queue.isEmpty())
            next_wakeup = 0ms;

        auto taskOpt = m_queue.take(next_wakeup);

        if (token.stop_requested())
            break;

        if (taskOpt.has_value() && *taskOpt) {
            try {
                (*taskOpt)();
            } catch (const std::exception& e) {
                JOB_LOG_ERROR("[AsyncEventLoop] Task exception: %s", e.what());
            } catch (...) {
                JOB_LOG_ERROR("[AsyncEventLoop] Task unknown exception");
            }
        }

        processTimers();
        processTasks();
    }
}

void AsyncEventLoop::runOnce()
{
    processTimers();
    processTasks();
}

} // job::threads

// CHECKPOINT: v1.2
