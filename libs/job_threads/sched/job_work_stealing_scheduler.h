#pragma once

#include <atomic>
#include <vector>
#include <memory>

#include "job_isched_policy.h"
#include "descr/job_task_descriptor.h"
#include "queue/job_mcmp_queue.h"

namespace job::threads {

class JobWorkStealingScheduler : public ISchedPolicy {
public:
    using Ptr = std::shared_ptr<JobWorkStealingScheduler>;
    using MPMCQueue = JobBoundedMPMCQueue<JobTaskDescriptor::Ptr>;
    explicit JobWorkStealingScheduler(std::size_t workerCount, std::size_t cap = 256);
    ~JobWorkStealingScheduler() override;

    [[nodiscard]] JobIDescriptor::Ptr createDescriptor(uint64_t id, int priority) override;
    void enqueue(JobIDescriptor::Ptr desc) override;
    [[nodiscard]] std::optional<uint64_t> next(uint32_t wid) override;
    void complete(uint64_t tid, bool ok) override;
    [[nodiscard]] size_t size() const noexcept override;
    void stop() override;

private:
    std::vector<std::unique_ptr<MPMCQueue>> m_queue;
    std::atomic<size_t>                     m_totalSize{0};
    std::atomic<size_t>                     m_submitCounter{0};
};

} // namespace job::threads
// CHECKPOINT v1.1
