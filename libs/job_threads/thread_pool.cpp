#include "thread_pool.h"

#include <job_logger.h>

namespace job::threads {

ThreadPool::Ptr ThreadPool::create(size_t threadCount, const JobThreadOptions &options)
{
    // Always construct under shared_ptr
    auto pool = std::shared_ptr<ThreadPool>(new ThreadPool{threadCount, options});

    // Attach watcher back to pool (safe now)
    pool->m_watcher->attachPool(pool);
    return pool;
}

ThreadPool::ThreadPool(size_t threadCount, const JobThreadOptions &options) :
    m_threadOptions(options)
{
    m_tasks = std::make_shared<TaskQueue>();
    m_watcher = std::make_shared<ThreadWatcher>();
    m_watcher->attachQueue(m_tasks);

    for (size_t i = 0; i < threadCount; i++) {
        auto worker = std::make_shared<JobThread>(m_threadOptions);
        worker->setRunFunction([this](std::stop_token token) {
            this->workerLoop(token);
        });

        const auto result = worker->start();
        switch (result) {
        case JobThread::StartResult::Started:
            m_watcher->addThread(worker, std::chrono::seconds(30), static_cast<int>(i));
            m_workers.push_back(worker);
            break;
        case JobThread::StartResult::SchedulingFailed:
            JOB_LOG_ERROR("[ThreadPool] Worker scheduling failed");
            break;
        case JobThread::StartResult::AffinityFailed:
            JOB_LOG_ERROR("[ThreadPool] Worker CPU affinity failed");
            break;
        default:
            JOB_LOG_ERROR("[ThreadPool] Failed to start worker thread");
            break;
        }
    }

    m_watcher->start();
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

ThreadPool::TaskMetrics ThreadPool::snapshotMetrics() const noexcept
{
    TaskMetrics metrics;

    if (!m_tasks)
        return metrics;

    metrics.total = m_tasks->size();
    metrics.byPriority.resize(10, 0);

    {
        // safe, scoped copy of queue state
        std::scoped_lock lock(m_tasks->m_mutex);
        auto tmp = m_tasks->m_queue; // copy the priority queue
        while (!tmp.empty()) {
            int prio = tmp.top().first;
            tmp.pop();

            if (prio < 0)
                prio = 0;

            if (static_cast<size_t>(prio) >= metrics.byPriority.size())
                metrics.byPriority.resize(prio + 1, 0);

            metrics.byPriority[prio]++;
        }
    }

    metrics.loadAvg = static_cast<double>(m_progress.load(std::memory_order_relaxed))
                      / static_cast<double>(m_workers.size());

    return metrics;
}


void ThreadPool::updateLoadAverage()
{
    // approximate "running tasks" as queue size + active workers
    const double current = static_cast<double>(m_tasks ? m_tasks->size() : 0);
    const double prev = m_progress.load(std::memory_order_relaxed);
    const double newLoad = (kLoadAlpha * prev) + ((1.0 - kLoadAlpha) * current);
    m_progress.store(static_cast<int>(newLoad), std::memory_order_relaxed);
}

void ThreadPool::shutdown()
{
    if (m_stopping.exchange(true))
        return;

    if (m_watcher) {
        m_watcher->stop();
        m_watcher.reset();
    }

    if (m_tasks)
        m_tasks->stop();

    for (auto &worker : m_workers) {
        worker->requestStop();
        worker->join();
    }

    m_workers.clear();
    m_tasks.reset();
}

size_t ThreadPool::taskCount() const noexcept
{
    return m_tasks ? m_tasks->size() : 0;
}

int ThreadPool::taskMin() const noexcept
{
    return m_minProgress;
}

int ThreadPool::taskMax() const noexcept
{
    return m_maxProgress;
}

void ThreadPool::setTaskRange(int min, int max)
{
    m_minProgress = min;
    m_maxProgress = max;
    m_progress.store(min, std::memory_order_relaxed);
}

void ThreadPool::workerLoop(std::stop_token token)
{
    while (!token.stop_requested()) {
        std::function<void()> task;
        if (auto optTask = m_tasks->take(std::chrono::milliseconds(100)))
            task = std::move(*optTask);

        if (!task) {
            if (m_stopping.load())
                return;

            continue;
        }

        try {
            task();
            m_progress.fetch_add(1, std::memory_order_relaxed);
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[ThreadPool] Worker task threw exception: %s", e.what());
        }
    }
}

} // namespace job::threads
