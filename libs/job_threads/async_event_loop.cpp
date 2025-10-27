#include "async_event_loop.h"

#include <iostream>
#include <chrono>
#include <pthread.h>

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
            std::cerr << "[AsyncEventLoop] join() threw: " << e.what() << '\n';
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

    core::JobTimer timer;
    timer.id = id;
    timer.nextFire = now + interval;
    timer.interval = interval;
    timer.repeat = repeat;
    timer.callback = std::move(callback);

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
    std::erase_if(m_timers, [id](const core::JobTimer &t) { return t.id == id; });
    return m_timers.size() != before;
}

void AsyncEventLoop::processTimers()
{
    const auto now = std::chrono::steady_clock::now();
    std::vector<std::function<void()>> ready;

    {
        std::scoped_lock lock(m_timerMutex);

        for (auto &timer : m_timers) {
            if (timer.nextFire <= now) {
                ready.push_back(timer.callback);
                if (timer.repeat)
                    timer.nextFire = now + timer.interval;
            }
        }

        // Remove one-shot timers that have fired
        std::erase_if(m_timers, [now](const core::JobTimer &t) {
            return !t.repeat && t.nextFire <= now;
        });
    }

    for (auto &cb : ready) {
        try {
            cb();
        } catch (const std::exception &e) {
            std::cerr << "[AsyncEventLoop] Timer exception: " << e.what() << '\n';
        } catch (...) {
            std::cerr << "[AsyncEventLoop] Timer unknown exception\n";
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
                std::cerr << "[AsyncEventLoop] Task exception: " << e.what() << '\n';
            } catch (...) {
                std::cerr << "[AsyncEventLoop] Unknown task exception\n";
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
            std::cerr << "[AsyncEventLoop] Unhandled exception: " << e.what() << '\n';
        } catch (...) {
            std::cerr << "[AsyncEventLoop] Unknown exception in runOnce\n";
        }
    }
}

} // namespace job::threads
