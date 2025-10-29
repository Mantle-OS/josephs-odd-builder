#include "thread_watcher.h"
#include "thread_pool.h"

#include <chrono>

#include <job_logger.h>
namespace job::threads {

// static constexpr auto kSummaryInterval = std::chrono::seconds(5);

ThreadWatcher::ThreadWatcher() = default;

ThreadWatcher::~ThreadWatcher() {
    stop();
}

void ThreadWatcher::addThread(const std::shared_ptr<JobThread> &thread,
                              core::JobTimer::Duration timeout, int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads.push_back( WatchedThread{
        .thread = thread,
        .startTime = core::JobTimer::Clock::now(),
        .timeout = timeout,
        .id = id
    });
}

void ThreadWatcher::start() {
    // already running ?
    if (m_running.exchange(true))
        return;

    JobThreadOptions opts = JobThreadOptions::normal();
    opts.name = "ThreadWatcher";

    m_monitorThread = std::make_shared<JobThread>(opts);
    m_monitorThread->setRunFunction([this](std::stop_token token) {
        monitorLoop(token);
    });

    auto ck = m_monitorThread->start();
    switch (ck) {
    case JobThread::StartResult::Started:
        break;
    case JobThread::StartResult::AlreadyRunning:
        JOB_LOG_DEBUG("Thread is already running");
        break;
    case JobThread::StartResult::SchedulingFailed:
        JOB_LOG_DEBUG("Could not set the Scheduling oif the thread");
        break;
    case JobThread::StartResult::AffinityFailed:
        JOB_LOG_DEBUG("Thread could not be pinned to the Core you tried to. If this is no realtime ignore");
        break;
    case JobThread::StartResult::ThreadError:
        JOB_LOG_DEBUG("Could not start the Thread Generic error");
        break;
    }
}

void ThreadWatcher::stop() {
    m_running.store(false);
    if (m_monitorThread) {
        m_monitorThread->requestStop();
        m_monitorThread->join();
        m_monitorThread.reset();
    }
}

bool ThreadWatcher::isRunning() const
{
    return m_running.load();
}

void ThreadWatcher::attachQueue(const std::shared_ptr<TaskQueue> &queue)
{
    std::scoped_lock lock(m_mutex);
    m_queue = queue;
}

void ThreadWatcher::attachPool(const std::shared_ptr<ThreadPool> &pool)
{
    std::scoped_lock lock(m_mutex);
    m_threadPool = pool;
}

void ThreadWatcher::monitorLoop(std::stop_token token)
{
    using namespace std::chrono_literals;

    auto lastSummary = core::JobTimer::Clock::now();

    while (!token.stop_requested()) {
        std::this_thread::sleep_for(100ms);

        std::lock_guard lock(m_mutex);
        const auto now = core::JobTimer::Clock::now();

        for (auto &wt : m_threads) {
            if (!wt.thread->isRunning())
                continue;

            const auto elapsed = std::chrono::duration_cast<core::JobTimer::Duration>(now - wt.startTime);
            if (elapsed > wt.timeout) {
                JOB_LOG_ERROR("[ThreadWatcher] Thread ID %d exceeded timeout of %d  ms (elapsed: %d ms)",
                              wt.id, wt.timeout.count(), elapsed.count());
                wt.thread->requestStop();
            }
        }

        if (auto q = m_queue.lock()) {
            if (q->stopped()) {
                JOB_LOG_ERROR("[ThreadWatcher] TaskQueue stopped");
            } else if (q->isEmpty()) {
                JOB_LOG_DEBUG("[ThreadWatcher] TaskQueue empty — possible starvation detected");
            }
        }

        static constexpr auto kSummaryInterval = std::chrono::seconds(5);
        if (now - lastSummary >= kSummaryInterval) {
            size_t threadCount = m_threads.size();
            size_t queueSize = 0;
            double loadAvg = 0.0;

            if (auto q = m_queue.lock())
                queueSize = q->size();

            // optional weak_ptr<ThreadPool> member
            if (auto pool = m_threadPool.lock())   {
                auto m = pool->snapshotMetrics();
                loadAvg = m.loadAvg;
            }

            JOB_LOG_DEBUG("[ThreadWatcher] Threads: {} | Queue size: {} | Load avg: {:.2f}",
                          threadCount, queueSize, loadAvg);

            lastSummary = now;
        }
    }
}

} // job::threads
