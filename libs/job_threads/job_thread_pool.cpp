#include "job_thread_pool.h"
#include <utility>

namespace job::threads {

ThreadPool::Ptr ThreadPool::create(ISchedPolicy::Ptr scheduler, size_t threadCount, const JobThreadOptions &options)
{
    if (!scheduler) {
        JOB_LOG_ERROR("[ThreadPool] Cannot create pool: ISchedPolicy is null.");
        return nullptr;
    }
    return std::shared_ptr<ThreadPool>(new ThreadPool{std::move(scheduler), threadCount, options});
}

ThreadPool::ThreadPool(
    ISchedPolicy::Ptr scheduler,
    size_t threadCount,
    const JobThreadOptions &options) :
    m_scheduler(std::move(scheduler)),
    m_threadOptions(options)
{
    m_watcher = std::make_shared<ThreadWatcher>();
    m_watcher->attachScheduler(m_scheduler);

    for (size_t i = 0; i < threadCount; i++) {
        auto worker = std::make_shared<JobThread>(m_threadOptions);
        worker->setRunFunction([this, worker_id = static_cast<uint32_t>(i)](std::stop_token token) {
            this->workerLoop(token, worker_id);
        });

        const auto result = worker->start();
        if (result == JobThread::StartResult::Started) {
            m_watcher->addThread(worker, std::chrono::seconds(30), static_cast<int>(i));
            m_workers.push_back(worker);
        } else {
            JOB_LOG_ERROR("[ThreadPool] Failed to start worker thread {}", i);
        }
    }
    m_watcher->start();
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

JobThreadMetrics ThreadPool::snapshotMetrics() const noexcept
{
    JobThreadMetrics metrics;
    if (!m_scheduler)
        return metrics;

    metrics.total = m_scheduler->size();

    metrics.loadAvg = static_cast<double>(m_progress.load(std::memory_order_relaxed));
    return metrics;
}

void ThreadPool::updateLoadAverage()
{
    const double current = static_cast<double>(m_progress.load(std::memory_order_relaxed));
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

    if (m_scheduler)
        m_scheduler->stop();

    for (auto &worker : m_workers)
        worker->requestStop();

    m_workerCondition.notify_all();

    for (auto &worker : m_workers)
        (void)worker->join();

    m_workers.clear();

    {
        std::lock_guard<std::mutex> lock(m_storageMutex);
        m_taskStorage.clear();
    }
    m_scheduler.reset();
}

size_t ThreadPool::workerCount() const noexcept
{
    return m_workers.size();
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

void ThreadPool::waitForIdle(std::chrono::milliseconds pollInterval) const
{
    if (!m_scheduler)
        return;

    for (;;) {
        if (m_scheduler->size() == 0 && (m_progress.load(std::memory_order_relaxed) == 0))
            break;
        std::this_thread::sleep_for(pollInterval);
    }
}

void ThreadPool::workerLoop(std::stop_token token, uint32_t worker_id)
{
    while (!token.stop_requested()) {
        std::function<void()> task_to_run;
        uint64_t task_id = 0;
        bool task_found = false;

        {
            std::unique_lock<std::mutex> lock(m_storageMutex);
            m_workerCondition.wait(lock, [&]{
                return m_scheduler->size() > 0 || token.stop_requested();
            });

            if (token.stop_requested())
                return;

            std::optional<uint64_t> id_opt = m_scheduler->next(worker_id);

            if (id_opt) {
                task_id = *id_opt;
                auto it = m_taskStorage.find(task_id);
                if (it != m_taskStorage.end()) {
                    task_to_run = std::move(it->second);
                    m_taskStorage.erase(it);
                    task_found = true;
                } else {
                    JOB_LOG_ERROR("[ThreadPool] Scheduler gave task ID {} but it was not in storage!", task_id);
                }
            }
        }

        if (task_found) {
            m_progress.fetch_add(1, std::memory_order_relaxed);
            bool success = true;
            try {
                task_to_run();
            } catch (const std::exception &e) {
                JOB_LOG_ERROR("[ThreadPool] Worker task {} threw exception: {}", task_id, e.what());
                success = false;
            } catch (...) {
                JOB_LOG_ERROR("[ThreadPool] Worker task {} threw unknown exception", task_id);
                success = false;
            }

            m_progress.fetch_sub(1, std::memory_order_relaxed);
            m_scheduler->complete(task_id, success);
        }
    }
}
} // job::threads
// CHECKPOINT: v1.3
