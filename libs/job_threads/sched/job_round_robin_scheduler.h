#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "job_isched_policy.h"
#include "descr/job_task_descriptor.h"

namespace job::threads {

class JobRoundRobinScheduler : public ISchedPolicy {
public:
    using Ptr = std::shared_ptr<JobRoundRobinScheduler>;

    JobRoundRobinScheduler(int threadCount){(void)threadCount;}
    ~JobRoundRobinScheduler() override;

    [[nodiscard]] JobIDescriptor::Ptr createDescriptor(uint64_t id, int priority) override;
    void enqueue(JobIDescriptor::Ptr desc) override;
    [[nodiscard]] std::optional<uint64_t> next(uint32_t wid) override;
    void complete(uint64_t tid, bool ok) override;
    [[nodiscard]] size_t size() const noexcept override;
    void stop() override;

private:
    mutable std::mutex                                  m_mutex;
    std::condition_variable                             m_condition;
    std::vector<std::queue<JobTaskDescriptor::Ptr>>     m_queues;
    size_t                                              m_totalSize{0};
    int                                                 m_currentPriority{0};
};

} // namespace job::threads
// CHECKPOINT v1.0
