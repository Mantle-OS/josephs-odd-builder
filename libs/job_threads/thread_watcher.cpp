#include "thread_watcher.h"
#include "thread_pool.h"

#include <chrono>
#include <iostream>

namespace job::threads {
// static constexpr auto kSummaryInterval = std::chrono::seconds(5);

ThreadWatcher::ThreadWatcher() = default;

ThreadWatcher::~ThreadWatcher() {
    stop();
}

void ThreadWatcher::addThread(const std::shared_ptr<JobThread> &thread,
                              Duration timeout, int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads.push_back( WatchedThread{
        .thread = thread,
        .startTime = Clock::now(),
        .timeout = timeout,
        .id = id
    });
}

void ThreadWatcher::start() {
    if (m_running.exchange(true))
        return; // already running

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
        std::cout << "Thread is already running\n";
        break;
    case JobThread::StartResult::SchedulingFailed:
        std::cout << "Could not set the Scheduling oif the thread\n";
        break;
    case JobThread::StartResult::AffinityFailed:
        std::cout << "Thread could not be pinned to the Core you tried to. If this is no realtime ignore\n";
        break;
    case JobThread::StartResult::ThreadError:
        std::cout << "Could not start the Thread Generic error\n";
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

    auto lastSummary = Clock::now();

    while (!token.stop_requested()) {
        std::this_thread::sleep_for(100ms);

        std::lock_guard lock(m_mutex);
        const auto now = Clock::now();

        for (auto &wt : m_threads) {
            if (!wt.thread->isRunning())
                continue;

            const auto elapsed = std::chrono::duration_cast<Duration>(now - wt.startTime);
            if (elapsed > wt.timeout) {
                std::cerr << "[ThreadWatcher] Thread ID " << wt.id
                          << " exceeded timeout of " << wt.timeout.count()
                          << " ms (elapsed: " << elapsed.count() << " ms)\n";
                wt.thread->requestStop();
            }
        }

        if (auto q = m_queue.lock()) {
            if (q->stopped()) {
                std::cerr << "[ThreadWatcher] TaskQueue stopped\n";
            } else if (q->isEmpty()) {
                std::cerr << "[ThreadWatcher] TaskQueue empty — possible starvation detected\n";
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

            std::cout << "[ThreadWatcher] Threads: " << threadCount
                      << " | Queue size: " << queueSize
                      << " | Load avg: " << std::fixed << std::setprecision(2)
                      << loadAvg << '\n';

            lastSummary = now;
        }
    }
}

} // job::threads
