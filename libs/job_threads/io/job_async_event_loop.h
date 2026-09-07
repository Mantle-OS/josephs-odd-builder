#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stop_token>
#include <vector>

#include <job_timer.h>

#include "job_task_queue.h"
#include "job_thread.h"
#include "jobthreads_export.h"

namespace job::threads {

class JOBTHREADS_EXPORT AsyncEventLoop {
public:
    AsyncEventLoop();
    virtual ~AsyncEventLoop() noexcept;

    AsyncEventLoop(const AsyncEventLoop &) = delete;
    AsyncEventLoop &operator=(const AsyncEventLoop &) = delete;
    AsyncEventLoop(AsyncEventLoop &&) = delete;
    AsyncEventLoop &operator=(AsyncEventLoop &&) = delete;

    void start(bool realTime = false);
    virtual void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    virtual void post(std::function<void()> task, int priority = 0);

    void runOnce();

    [[nodiscard]] std::uint64_t postDelayed(std::function<void()> task,
                                            std::chrono::milliseconds delay);

    [[nodiscard]] std::uint64_t addTimer(std::function<void()> callback,
                                         std::chrono::milliseconds interval,
                                         bool repeat = false);

    [[nodiscard]] bool cancelTimer(std::uint64_t id);

    static AsyncEventLoop &globalLoop()
    {
        static AsyncEventLoop loop;
        static std::once_flag once;
        std::call_once(once, [] { loop.start(); });
        return loop;
    }

protected:
    virtual void loop(std::stop_token token,
                      std::chrono::milliseconds idleHeartbeat);

    void processTimers();
    void processTasks();

    [[nodiscard]] std::chrono::milliseconds calculateNextWakeup() const;

    TaskQueue m_queue;
    JobThread::Ptr m_thread;

    mutable std::mutex m_timerMutex;
    std::vector<core::JobTimer> m_timers;

    std::atomic<bool> m_running{false};
    std::atomic<std::uint64_t> m_nextTimerId{1};
};

} // namespace job::threads