#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

#include <job_timer.h>

#include "job_thread.h"

#include "task_queue.h"

namespace job::threads {

class ThreadPool;
class ThreadWatcher {

public:
    struct WatchedThread {
        std::shared_ptr<JobThread> thread;
        core::JobTimer::Clock::time_point startTime;
        core::JobTimer::Duration timeout;
        int id;
    };

    ThreadWatcher();
    ~ThreadWatcher();

    ThreadWatcher(const ThreadWatcher &) = delete;
    ThreadWatcher &operator=(const ThreadWatcher &) = delete;

    void addThread(const std::shared_ptr<JobThread> &thread,
                   core::JobTimer::Duration timeout,
                   int id = -1);

    void attachQueue(const std::shared_ptr<TaskQueue> &queue);
    void attachPool(const std::shared_ptr<ThreadPool> &pool);
    void start();
    void stop();

    [[nodiscard]] bool isRunning() const;

private:
    void monitorLoop(std::stop_token token);
    std::vector<WatchedThread> m_threads;

    std::weak_ptr<TaskQueue> m_queue;
    std::weak_ptr<ThreadPool> m_threadPool;

    mutable std::mutex m_mutex;
    std::atomic<bool> m_running{false};
    std::shared_ptr<JobThread> m_monitorThread;

};
} // namespace job::threads
