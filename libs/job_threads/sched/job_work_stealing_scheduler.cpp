#include "job_work_stealing_scheduler.h"
#include <memory>
#include <job_logger.h>
namespace job::threads {

JobWorkStealingScheduler::JobWorkStealingScheduler(std::size_t workerCount, std::size_t cap)
{
    if (workerCount == 0)
        workerCount = 1;

    for(size_t i = 0; i < workerCount; ++i)
        m_queue.emplace_back(std::make_unique<MPMCQueue>(cap));
}

JobWorkStealingScheduler::~JobWorkStealingScheduler()
{
    stop();
}

[[nodiscard]] JobIDescriptor::Ptr JobWorkStealingScheduler::createDescriptor(uint64_t id, int priority)
{
    return std::make_shared<JobTaskDescriptor>(id, priority);
}

void JobWorkStealingScheduler::enqueue(JobIDescriptor::Ptr desc)
{
    if (m_stopped.load())
        return;

    auto wsDesc = std::dynamic_pointer_cast<JobTaskDescriptor>(desc);
    if (!wsDesc){
        JOB_LOG_WARN("[JobWorkStealingScheduler] Non task descriptor enqueued to work-stealing scheduler; dropping (id={}).",
                     desc ? desc->id() : 0);
        return;
    }

    size_t targetQueue = m_submitCounter.fetch_add(1) % m_queue.size();

    if (m_queue[targetQueue]->push(std::move(wsDesc)))
        m_totalSize.fetch_add(1);
    else
        JOB_LOG_WARN("[JobWorkStealingScheduler] push failed, likely because queue was closed");
}

[[nodiscard]] std::optional<uint64_t> JobWorkStealingScheduler::next(uint32_t wid)
{
    if (m_totalSize.load() == 0)
        return std::nullopt;

    JobTaskDescriptor::Ptr task;

    if (m_queue[wid]->tryPop(task)) {
        m_totalSize.fetch_sub(1);
        return task->id();
    }

    const size_t numWorkers = m_queue.size();
    for (size_t i = 1; i < numWorkers; ++i) {
        size_t stealTargetId = (wid + i) % numWorkers;
        if (m_queue[stealTargetId]->tryPop(task)) {
            m_totalSize.fetch_sub(1);
            return task->id();
        }
    }
    return std::nullopt;
}

void JobWorkStealingScheduler::complete(uint64_t /*tid*/, bool /*ok*/)
{
    // No-op for this scheduler
}

[[nodiscard]] size_t JobWorkStealingScheduler::size() const noexcept
{
    return m_totalSize.load();
}

void JobWorkStealingScheduler::stop()
{
    ISchedPolicy::stop();
    for (auto &q : m_queue)
        q->close();
}

} // namespace job::threads
// CHECKPOINT v1.1
