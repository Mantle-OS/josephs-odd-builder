#pragma once

#include "descr/job_task_descriptor.h"
#include <chrono>

namespace job::threads {

using namespace std::chrono_literals;

class JobSporadicDescriptor : public JobTaskDescriptor {
public:
    using Ptr = std::shared_ptr<JobSporadicDescriptor>;
    JobSporadicDescriptor( uint64_t id, int priority, std::chrono::steady_clock::time_point deadline, std::chrono::microseconds wcet) :
        JobTaskDescriptor(id, priority),
        m_deadline(deadline),
        m_wcet(wcet)
    {
    }

    [[nodiscard]] std::chrono::steady_clock::time_point deadline() const noexcept
    {
        return m_deadline;
    }
    [[nodiscard]] std::chrono::microseconds wcet() const noexcept
    {
            return m_wcet;
    }

    [[nodiscard]] double utilization() const noexcept
    {
        auto relativeDeadline = std::chrono::duration_cast<std::chrono::microseconds>(
            m_deadline - std::chrono::steady_clock::now()
            );

        if (relativeDeadline.count() <= 0 || m_wcet.count() <= 0)
            return 0.0; // NO div 0

        return static_cast<double>(m_wcet.count()) /
               static_cast<double>(relativeDeadline.count());
    }

private:
    std::chrono::steady_clock::time_point   m_deadline;
    std::chrono::microseconds               m_wcet; // worst case elipsed time
};

} // namespace job::threads
// CHECKPOINT 1.0
