#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#include "job_isched_policy.h"
#include "descr/job_sporadic_descriptor.h"

namespace job::threads {

class JobSporadicScheduler : public ISchedPolicy {
public:
    using Ptr = std::shared_ptr<JobSporadicScheduler>;

    explicit JobSporadicScheduler(unsigned capacity = 1) :
        m_capacity(capacity ? capacity : 1)
    {

    }
    ~JobSporadicScheduler() override;

    [[nodiscard]] JobIDescriptor::Ptr createDescriptor(uint64_t id, int priority) override;
    [[nodiscard]] bool admit(uint32_t wid, const JobIDescriptor& desc) override;
    void enqueue(JobIDescriptor::Ptr desc) override;
    [[nodiscard]] std::optional<uint64_t> next(uint32_t wid) override;
    void complete(uint64_t tid, bool ok) override;
    [[nodiscard]] size_t size() const noexcept override;
    void stop() override;

private:
    using SporadicDescPtr = JobSporadicDescriptor::Ptr;

    struct DeadlineComparator {
        bool operator()(const SporadicDescPtr& a, const SporadicDescPtr& b) const {
            return a->deadline() > b->deadline(); // earliest deadline at top
        }
    };

    // admission check for ready jobs under EDF on 1 CPU. (Supply scales by m_capacity; for m>1 it's a safe—necessary—condition.)
    bool canAdmitUnlocked(std::chrono::steady_clock::time_point now,
                          std::chrono::steady_clock::time_point new_deadline,
                          std::chrono::microseconds new_wcet) const;

    // EDF queue of pending jobs
    std::priority_queue<SporadicDescPtr, std::vector<SporadicDescPtr>, DeadlineComparator> m_queue;

    // Jobs handed out by next() and not yet completed
    std::unordered_map<uint64_t, SporadicDescPtr> m_inflight;

    // Reservations created by admit() and consumed by enqueue()
    struct Reservation {
        std::chrono::steady_clock::time_point   deadline;
        std::chrono::microseconds               wcet;
    };
    std::unordered_map<uint64_t, Reservation> m_reserved;

    unsigned m_capacity{1};

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

};

} // namespace job::threads
// CHECKLPOINT v1.1
