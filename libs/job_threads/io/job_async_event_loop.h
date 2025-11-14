#pragma once

#include <atomic>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>
#include <stop_token>

#include <job_timer.h>

#include "job_thread.h"
#include "job_task_queue.h"

namespace job::threads {

class AsyncEventLoop {
public:
    AsyncEventLoop();
    virtual ~AsyncEventLoop() noexcept;

    void start(bool realTime = false);
    virtual void stop();
    [[nodiscard]] bool isRunning() const;

    virtual void post(std::function<void()> task, int priority = 0);
    void runOnce();

    uint64_t postDelayed(std::function<void()> task, std::chrono::milliseconds delay);

    uint64_t addTimer(std::function<void()> callback,
                      std::chrono::milliseconds interval,
                      bool repeat = false);

    bool cancelTimer(uint64_t id);

    static AsyncEventLoop &globalLoop()
    {
        static AsyncEventLoop loop;
        static std::once_flag once;
        std::call_once(once, [] { loop.start(); });
        return loop;
    }

protected:
    virtual void loop(std::stop_token token, std::chrono::milliseconds idle_heartbeat);
    void processTimers();
    void processTasks();

    std::chrono::milliseconds calculateNextWakeup() const;

    TaskQueue                       m_queue;
    JobThread::Ptr                  m_thread;
    mutable std::mutex              m_timerMutex;
    std::vector<core::JobTimer>     m_timers;
    std::atomic<bool>               m_running{false};
    std::atomic<uint64_t>           m_nextTimerId{1};

private:

};

} // job::threads
// CHECKPOINT: v1.1
