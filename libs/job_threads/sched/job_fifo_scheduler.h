#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

#include "job_isched_policy.h"
#include "job_task_descriptor.h"

namespace job::threads {

class FifoScheduler : public ISchedPolicy {
public:
    using Ptr = std::shared_ptr<FifoScheduler>;

    FifoScheduler() = default;
    ~FifoScheduler() override;

    JobIDescriptor::Ptr createDescriptor(uint64_t id, int priority) override
    {
        return std::make_shared<JobTaskDescriptor>(id, priority);
    }

    // We want to pass in the TASK Descriptor not the interface ?
    void enqueue(JobIDescriptor::Ptr desc) override;
    [[nodiscard]] std::optional<uint64_t> next(uint32_t wid) override;
    void complete(uint64_t tid, bool ok) override;
    [[nodiscard]] size_t size() const noexcept override;

    void stop() override;

private:
    mutable std::mutex m_mutex;

    // The que
    std::queue<JobTaskDescriptor::Ptr> m_queue;
    std::condition_variable m_condition;
};

} // namespace job::threads
// CHECKPOINT: v1.0
