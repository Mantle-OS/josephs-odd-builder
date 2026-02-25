#include "job_fifo_scheduler.h"
#include <job_logger.h>

namespace job::threads {
FifoScheduler::~FifoScheduler()
{
    stop();
}

void FifoScheduler::enqueue(JobIDescriptor::Ptr desc)
{
    if (m_stopped.load())
        return;

    auto taskDesc = std::dynamic_pointer_cast<JobTaskDescriptor>(desc);
    if (!taskDesc){
        JOB_LOG_WARN("[FifoScheduler] Non task descriptor enqueued to fifo scheduler; dropping (id={}).",
                     desc ? desc->id() : 0);
        return;
    }

    if(!taskDesc){
        JOB_LOG_ERROR("[FifoScheduler] faild to cast the base type from JobIDescriptor to a JobTaskDescriptor");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(taskDesc));
    }
    m_condition.notify_one();
}

std::optional<uint64_t> FifoScheduler::next(uint32_t /*wid*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this]{
        return !m_queue.empty() || m_stopped.load();
    });

    if (m_stopped.load() && m_queue.empty())
        return std::nullopt;

    if (m_queue.empty())
        return std::nullopt;

    JobTaskDescriptor::Ptr desc = std::move(m_queue.front());
    m_queue.pop();

    return desc->id();
}

void FifoScheduler::complete(uint64_t /*tid*/, bool /*ok*/)
{
    // No-op for a simple FIFO scheduler. A TaskGraph scheduler would use this to unlock dependents.
}

size_t FifoScheduler::size() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void FifoScheduler::stop()
{
    ISchedPolicy::stop();
    m_condition.notify_all();
}

} // namespace job::threads

