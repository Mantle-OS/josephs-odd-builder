#include "job_thread_watcher.h"

#include <job_logger.h>

#include "job_thread_pool.h"

namespace job::threads {

using namespace std::chrono_literals;

ThreadWatcher::~ThreadWatcher() noexcept
{
    stop();
}

void ThreadWatcher::attachScheduler(const std::shared_ptr<ISchedPolicy> &scheduler)
{
    std::scoped_lock lock(m_mutex);
    m_scheduler = scheduler;
}

void ThreadWatcher::addThread(const JobThread::Ptr &thread,
                              std::chrono::milliseconds timeout, int id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads.push_back( WatchedThread{
        .thread = thread,
        .startTime = std::chrono::steady_clock::now(),
        .timeout = timeout,
        .id = id
    });
}

void ThreadWatcher::start(bool realTime)
{
    // already running ?
    if (m_running.exchange(true))
        return;

    JobThreadOptions opts = realTime ?
                                JobThreadOptions::realtimeDefault() :
                                JobThreadOptions::normal();
    std::snprintf(opts.name.data(), opts.name.size(), "ThreadWatcher");

    m_monitorThread = std::make_shared<JobThread>(opts);

    auto safe_heartbeat = std::max<uint16_t>(opts.heartbeat, 10);
    auto loop_heartbeat = std::chrono::milliseconds(safe_heartbeat) ;//(opts.heartbeat);
    m_monitorThread->setRunFunction([this, loop_heartbeat](std::stop_token token) {
        monitorLoop(token, loop_heartbeat);
    });

    switch (m_monitorThread->start()) {
    case JobThread::StartResult::Started:
        break;
    case JobThread::StartResult::AlreadyRunning:
        JOB_LOG_INFO("[ThreadWatcher] Start Thread is already running");
        break;
    case JobThread::StartResult::SchedulingFailed:
        JOB_LOG_INFO("[ThreadWatcher] Start Could not set the Scheduling of the thread");
        break;
    case JobThread::StartResult::AffinityFailed:
        JOB_LOG_INFO("[ThreadWatcher] Start Thread could not be pinned to the Core you tried to. If this is no realtime ignore");
        break;
    case JobThread::StartResult::ThreadError:
        JOB_LOG_INFO("[ThreadWatcher] Start Could not start the Thread Generic error");
        break;
    }
}

void ThreadWatcher::stop() {
    m_running.store(false);
    if (m_monitorThread) {
        m_monitorThread->requestStop();
        (void)m_monitorThread->join();
        m_monitorThread.reset();
    }
}

bool ThreadWatcher::isRunning() const noexcept
{
    return m_running.load();
}

void ThreadWatcher::attachPool(const std::shared_ptr<ThreadPool> &pool)
{
    std::scoped_lock lock(m_mutex);
    m_threadPool = pool;
}

void ThreadWatcher::monitorLoop(std::stop_token token, std::chrono::milliseconds loop_heartbeat)
{
    auto lastSummary = std::chrono::steady_clock::now();

    while (!token.stop_requested()) {
        std::this_thread::sleep_for(loop_heartbeat);

        std::lock_guard lock(m_mutex);
        const auto now = std::chrono::steady_clock::now();

        for (auto &wt : m_threads) {
            if (!wt.thread->isRunning())
                continue;

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - wt.startTime);
            if (elapsed > wt.timeout) {
                // JOB_LOG_ERROR("[ThreadWatcher] Thread ID {} exceeded timeout of {} ms (elapsed: {} ms)",
                //               wt.id, wt.timeout.count(), elapsed.count());
                wt.thread->requestStop();
            }
        }

        if (auto s = m_scheduler.lock()){
            if (s->stopped())
                JOB_LOG_ERROR("[ThreadWatcher] Scheduler stopped unexpectedly");
        }
        if (now - lastSummary >= m_summaryInterval) {
            // size_t threadCount = m_threads.size();
            // size_t queueSize = 0;
            // double loadAvg = 0.0;
            // if (auto s = m_scheduler.lock())
                // queueSize = s->size();

            if (auto pool = m_threadPool.lock()) {
                auto m = pool->snapshotMetrics();
                // loadAvg = m.loadAvg;
            }

            // JOB_LOG_INFO("[ThreadWatcher] Threads: {}", threadCount);
            // JOB_LOG_INFO("[ThreadWatcher] Queue size: {}",  queueSize);
            // JOB_LOG_INFO("[ThreadWatcher] Load avg: {} ", loadAvg);;

            lastSummary = now;
        }
    }
}
} // job::threads

