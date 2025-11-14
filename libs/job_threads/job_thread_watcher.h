#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

#include "job_isched_policy.h"
#include "job_thread.h"
#include "job_task_queue.h"

namespace job::threads {

class ThreadPool;
class ThreadWatcher {
public:

    using Ptr  = std::shared_ptr<ThreadWatcher>;
    using WPtr = std::weak_ptr<ThreadWatcher>;

    struct WatchedThread {
        std::shared_ptr<JobThread> thread;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::milliseconds timeout;
        int id;
    };

    ThreadWatcher() = default;
    ~ThreadWatcher() noexcept;

    ThreadWatcher(const ThreadWatcher &) = delete;
    ThreadWatcher &operator=(const ThreadWatcher &) = delete;

    void addThread(const JobThread::Ptr &thread,
                   std::chrono::milliseconds timeout,
                   int id = -1);

    void attachScheduler(const std::shared_ptr<ISchedPolicy> &scheduler);
    void attachPool(const std::shared_ptr<ThreadPool> &pool);
    void start(bool realTime = false);
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    void setSummaryInterval(std::chrono::milliseconds interval) {
        std::scoped_lock lock(m_mutex);
        m_summaryInterval = interval;
    }

private:
    void monitorLoop(std::stop_token token, std::chrono::milliseconds loop_heartbeat);

    std::vector<WatchedThread>      m_threads;
    ISchedPolicy::WPtr              m_scheduler;
    std::weak_ptr<ThreadPool>       m_threadPool;
    mutable std::mutex              m_mutex;
    std::atomic<bool>               m_running{false};
    JobThread::Ptr                  m_monitorThread;
    std::chrono::milliseconds       m_summaryInterval{std::chrono::seconds(5)};
};
} // job::threads
// CHECKPOINT: v1.2
