#include "async_event_loop.h"

#include <chrono>
#include <pthread.h>

#include <job_logger.h>

// FIXME new logger and e.what()

namespace job::threads {

AsyncEventLoop::AsyncEventLoop() = default;

AsyncEventLoop::~AsyncEventLoop()
{
    stop();
}

void AsyncEventLoop::start()
{
    if (m_running.exchange(true))
        return;

    m_thread = std::jthread([this](std::stop_token token) {
        pthread_setname_np(pthread_self(), "AsyncLoop");
        this->loop(token);
    });
}

void AsyncEventLoop::stop()
{
    if (!m_running.exchange(false))
        return;

    {
        std::scoped_lock lock(m_timerMutex);
        m_timers.clear();
    }

    // Unblock any waiting take()
    m_queue.stop();

    if (m_thread.joinable()) {
        m_thread.request_stop();

        // Try a normal join first
        try {
            m_thread.join();
        } catch (const std::system_error &e) {
            JOB_LOG_ERROR("[AsyncEventLoop] join() threw: %s \n", e.what());
        }
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

    core::JobTimer timer{
        id,
        (now + interval),
        interval,
        repeat,
        true,
        std::move(callback)
    };

    {
        std::scoped_lock lock(m_timerMutex);
        m_timers.push_back(std::move(timer));
    }

    // Wake up the loop if it's sleeping
    m_queue.post([] {}, kDefaultPriority);
    return id;
}

bool AsyncEventLoop::cancelTimer(uint64_t id)
{
    std::scoped_lock lock(m_timerMutex);
    const auto before = m_timers.size();
    std::erase_if(m_timers, [id](const core::JobTimer &t) {
        return t.id() == id;
    });
    return m_timers.size() != before;
}

void AsyncEventLoop::processTimers()
{
    const auto now = std::chrono::steady_clock::now();
    std::vector<std::function<void()>> ready;

    {
        std::scoped_lock lock(m_timerMutex);

        for (auto &timer : m_timers) {
            if (timer.next() <= now) {
                ready.push_back(timer.callback());
                if (timer.repeat())
                    timer.set_next(now + timer.interval());
            }
        }

        // Remove one-shot timers that have fired
        std::erase_if(m_timers, [now](const core::JobTimer &t) {
            return !t.repeat() && t.next() <= now;
        });
    }

    for (auto &cb : ready) {
        try {
            cb();
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[AsyncEventLoop] Timer exception: %s \n", e.what());
        } catch (...) {
            JOB_LOG_ERROR("[AsyncEventLoop] Timer unknown exception\n");
        }
    }
}

void AsyncEventLoop::loop(std::stop_token token)
{
    using namespace std::chrono_literals;

    while (!token.stop_requested()) {
        auto taskOpt = m_queue.take(std::chrono::milliseconds(50));

        if (taskOpt.has_value()) {
            try {
                taskOpt.value()();
            } catch (const std::exception &e) {
                JOB_LOG_ERROR("[AsyncEventLoop] Task exception: %s \n", e.what());
            } catch (...) {
                JOB_LOG_ERROR("[AsyncEventLoop] Unknown task exception\n");
            }
        }

        processTimers();
    }
}

void AsyncEventLoop::runOnce()
{
    processTimers();
    auto taskOpt = m_queue.take(std::chrono::milliseconds(10));
    if (taskOpt.has_value()) {
        try {
            taskOpt.value()();
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[AsyncEventLoop] Unhandled exception: %s \n", e.what());
        } catch (...) {
            JOB_LOG_ERROR("[AsyncEventLoop] Unknown exception in runOnce\n");
        }
    }
}

} // namespace job::threads
