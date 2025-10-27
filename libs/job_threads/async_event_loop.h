#pragma once

#include "task_queue.h"
#include <atomic>
#include <thread>
#include <functional>
#include <stop_token>
#include <chrono>
#include <mutex>
#include <vector>

#include <job_timer.h>

namespace job::threads {

class AsyncEventLoop final {
public:
    AsyncEventLoop();
    ~AsyncEventLoop();

    void start();
    void stop();
    [[nodiscard]] bool isRunning() const;

    void post(std::function<void()> task, int priority = kDefaultPriority);
    void runOnce();

    uint64_t postDelayed(std::function<void()> task, std::chrono::milliseconds delay);
    uint64_t addTimer(std::function<void()> callback,
                      std::chrono::milliseconds interval,
                      bool repeat = false);
    bool cancelTimer(uint64_t id);

    static constexpr int kDefaultPriority = 0;

    static AsyncEventLoop &globalLoop()
    {
        static AsyncEventLoop loop;
        static std::once_flag once;
        std::call_once(once, [] { loop.start(); });
        return loop;
    }

private:
    void loop(std::stop_token token);
    void processTimers();

    std::jthread                     m_thread;
    TaskQueue                        m_queue;
    std::atomic<bool>                m_running{false};

    std::mutex                       m_timerMutex;
    std::vector<core::JobTimer>      m_timers;
    std::atomic<uint64_t>            m_nextTimerId{1};
};

} // namespace job::threads
