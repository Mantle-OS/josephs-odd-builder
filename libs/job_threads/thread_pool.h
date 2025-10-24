#pragma once

#include <vector>
#include <memory>
#include <future>
#include <atomic>

#include "job_thread.h"
#include "task_queue.h"
#include "thread_options.h"
#include "thread_watcher.h"

namespace job::threads {
constexpr double kLoadAlpha = 0.75;   // EWMA decay constant

class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
public:
    using Ptr = std::shared_ptr<ThreadPool>;

    static Ptr create(size_t threadCount = std::thread::hardware_concurrency(),
                      const JobThreadOptions &options = JobThreadOptions::normal());


    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    struct TaskMetrics {
        std::vector<size_t> byPriority;   // e.g. index 0 = prio 0 count
        double loadAvg{0.0};
        size_t total{0};
    };
    TaskMetrics snapshotMetrics() const noexcept;
    void updateLoadAverage();


    void shutdown();

    template <typename Func, typename... Args>
    auto submit(Func &&f, Args &&...args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using ReturnType = std::invoke_result_t<Func, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

        std::future<ReturnType> result = task->get_future();

        if (m_stopping.load())
            throw std::runtime_error("[ThreadPool] Stopping, cannot submit new tasks");

        m_tasks->emplace([task]() { (*task)(); }, 0);

        return result;
    }

    [[nodiscard]] size_t taskCount() const noexcept;
    [[nodiscard]] int taskMin() const noexcept;
    [[nodiscard]] int taskMax() const noexcept;

    void setTaskRange(int min, int max);

protected:
    ThreadPool(size_t threadCount = std::thread::hardware_concurrency(),
               const JobThreadOptions &options = JobThreadOptions::normal());
    void workerLoop(std::stop_token token);

private:
    std::vector<std::shared_ptr<JobThread>> m_workers;
    std::shared_ptr<TaskQueue> m_tasks;               // now heap-allocated so watcher can hold weak_ptr
    std::shared_ptr<ThreadWatcher> m_watcher;

    std::atomic<bool> m_stopping{false};

    std::atomic<int> m_progress{0};
    int m_minProgress{0};
    int m_maxProgress{0};

    JobThreadOptions m_threadOptions;
};

} // namespace job::threads
