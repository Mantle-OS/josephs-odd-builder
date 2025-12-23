#pragma once

#include <vector>
#include <memory>
#include <future>
#include <atomic>

#include <job_logger.h>

#include "job_thread.h"
#include "job_thread_options.h"
#include "job_thread_watcher.h"
#include "job_semaphore.h"
#include "sched/job_isched_policy.h"
#include "descr/job_task_descriptor.h"

namespace job::threads {

// EWMA decay
constexpr double kLoadAlpha = 0.75;

class ThreadPool {
public:
    using WPtr = std::weak_ptr<ThreadPool>;
    using Ptr  = std::shared_ptr<ThreadPool>;
    using TaskFunction = std::function<void()>;

    static Ptr create(ISchedPolicy::Ptr scheduler,
                      size_t threadCount = std::thread::hardware_concurrency(),
                      const JobThreadOptions &options = JobThreadOptions::normal());


    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    JobThreadMetrics snapshotMetrics() const noexcept;
    void updateLoadAverage();

    void shutdown();

    template <typename Func, typename... Args>
    auto submit(JobIDescriptor::Ptr desc, Func &&f, Args &&...args) -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using ReturnType = std::invoke_result_t<Func, Args...>;

        if (m_stopping.load(std::memory_order_relaxed) || !m_scheduler) {
            JOB_LOG_ERROR("[ThreadPool] Stopping or no scheduler, cannot submit new tasks");
            return {};
        }

        if (!m_scheduler->admit(m_workers.size(), *desc)) {
            // JOB_LOG_WARN("[ThreadPool] Scheduler rejected task ID {}", desc->id());
            return {};
        }

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
            );

        std::future<ReturnType> result = task->get_future();

        uint64_t id = desc->id();
        {
            TaskShard &shard = getShard(id);
            std::lock_guard<std::mutex> lock(shard.mutex);
            shard.map[id] = [task](){ (*task)(); };
        }

        m_scheduler->enqueue(std::move(desc));
        (void)m_workSemaphore.post();

        return result;
    }

    template <typename Func, typename... Args>
    auto submit(int priority, Func &&f, Args &&...args) -> std::future<std::invoke_result_t<Func, Args...>>
    {
        if (m_stopping.load() || !m_scheduler) {
            JOB_LOG_ERROR("[ThreadPool] Stopping or no scheduler, cannot submit new tasks");
            return {};
        }

        uint64_t id = m_nextTaskId.fetch_add(1, std::memory_order_relaxed);
        auto desc = m_scheduler->createDescriptor(id, priority);
        if (!desc) {
            JOB_LOG_ERROR("[ThreadPool] Scheduler failed to create a descriptor for task {}", id);
            return {};
        }

        return submit(std::move(desc), std::forward<Func>(f), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    auto submit(Func &&f, Args &&...args) -> std::future<std::invoke_result_t<Func, Args...>>
    {
        // Who's the boss ... Martin Cohan(https://en.wikipedia.org/wiki/Who%27s_the_Boss%3F)
        return submit(0, std::forward<Func>(f), std::forward<Args>(args)...);
    }

    [[nodiscard]] size_t workerCount() const noexcept;
    [[nodiscard]] int taskMin() const noexcept;
    [[nodiscard]] int taskMax() const noexcept;

    void setTaskRange(int min, int max);
    void waitForIdle(std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10)) const;

    static bool inWorkerThread() noexcept;
protected:
    ThreadPool(ISchedPolicy::Ptr scheduler, size_t threadCount = std::thread::hardware_concurrency(),
               const JobThreadOptions &options = JobThreadOptions::normal());

    void workerLoop(std::stop_token token, uint32_t worker_id);
private:


    static constexpr size_t kShardCount = 64;

    struct alignas(64) TaskShard {
        mutable std::mutex mutex;
        std::unordered_map<uint64_t, TaskFunction> map;
    };

    std::array<TaskShard, kShardCount> m_taskShards;

    // Helper to pick the right bucket
    TaskShard &getShard(uint64_t taskId)
    {
        // Simple hash: ID modulo Count
        return m_taskShards[taskId % kShardCount];
    }

    JobSem m_workSemaphore;
    std::atomic<int> m_progress{0};
    std::atomic<double> m_loadAvg{0.0};

    std::vector<JobThread::Ptr>                 m_workers;
    ISchedPolicy::Ptr                           m_scheduler;
    std::atomic<uint64_t>                       m_nextTaskId{1};
    ThreadWatcher::Ptr                          m_watcher;
    std::atomic<bool>                           m_stopping{false};
    int                                         m_minProgress{0};
    int                                         m_maxProgress{0};
    JobThreadOptions                            m_threadOptions;
};

} // job::threads
// CHECKPOINT: v1.4


