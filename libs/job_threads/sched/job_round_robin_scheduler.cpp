#include "job_round_robin_scheduler.h"
#include <memory>
#include <job_logger.h>
namespace job::threads {

JobRoundRobinScheduler::~JobRoundRobinScheduler()
{
    stop();
}

JobIDescriptor::Ptr JobRoundRobinScheduler::createDescriptor(uint64_t id, int priority)
{
    return std::make_shared<JobTaskDescriptor>(id, priority);
}

void JobRoundRobinScheduler::enqueue(JobIDescriptor::Ptr desc)
{
    if (m_stopped.load())
        return;

    auto rrDesc = std::dynamic_pointer_cast<JobTaskDescriptor>(desc);
    if (!rrDesc){
        JOB_LOG_WARN("[RoundRobinScheduler] Non task descriptor enqueued to rr scheduler; dropping (id={}).",
                     desc ? desc->id() : 0);
        return;
    }

    int priority = rrDesc->priority();
    if (priority < 0)
        priority = 0; // Ensure non-negative

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (static_cast<size_t>(priority) >= m_queues.size())
            m_queues.resize(priority + 1);

        m_queues[priority].push(std::move(rrDesc));
        m_totalSize++;
    }

    m_condition.notify_one();
}

std::optional<uint64_t> JobRoundRobinScheduler::next(uint32_t /*wid*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this]{
        return m_totalSize > 0 || m_stopped.load();
    });

    if (m_stopped.load() && m_totalSize == 0)
        return std::nullopt;

    if (m_totalSize == 0)
        return std::nullopt;

    const size_t num_queues = m_queues.size();
    if (num_queues == 0)
        return std::nullopt; // No queues exist

    for (size_t i = 0; i < num_queues; ++i) {
        int prio_to_check = (m_currentPriority + i) % num_queues;

        if (!m_queues[prio_to_check].empty()) {
            auto desc = std::move(m_queues[prio_to_check].front());
            m_queues[prio_to_check].pop();
            m_totalSize--;
            m_currentPriority = (prio_to_check + 1) % num_queues;
            return desc->id();
        }
    }


    return std::nullopt; // Should be unreachable if m_totalSize > 0
}

void JobRoundRobinScheduler::complete(uint64_t /*tid*/, bool /*ok*/)
{
    // No-op for this scheduler
}

 size_t JobRoundRobinScheduler::size() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalSize;
}

void JobRoundRobinScheduler::stop()
{
    ISchedPolicy::stop();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_totalSize = 0;
        m_queues.clear();
    }
    m_condition.notify_all();
}

} // namespace job::threads
// CHECKPOINT v1.0
