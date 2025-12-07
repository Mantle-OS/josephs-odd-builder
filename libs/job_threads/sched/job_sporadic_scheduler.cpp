#include "job_sporadic_scheduler.h"
#include <job_logger.h>
#include <algorithm>

namespace job::threads {

JobSporadicScheduler::~JobSporadicScheduler()
{
    stop();
}

JobIDescriptor::Ptr JobSporadicScheduler::createDescriptor(uint64_t id, int priority)
{
    return std::make_shared<JobSporadicDescriptor>(
        id, priority,
        std::chrono::steady_clock::now(),
        std::chrono::microseconds(0));
}

// no reservation
bool JobSporadicScheduler::admit(uint32_t /*wid*/, const JobIDescriptor& base)
{
    const auto* spor = dynamic_cast<const JobSporadicDescriptor*>(&base);
    // Non-sporadic descriptors: treat as 0 cost and admit.
    if (!spor)
        return true;

    using clock = std::chrono::steady_clock;
    const auto now  = clock::now();
    const auto wcet = spor->wcet();
    const auto dl   = spor->deadline();

    if (wcet.count() <= 0) return true;
    if (dl <= now) {
        JOB_LOG_WARN("[Sporadic] Task {} rejected: deadline already <= now.", base.id());
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Include existing pending, in-flight, and *reserved* work.
    if (!canAdmitUnlocked(now, dl, wcet))
        return false;

    // Reserve capacity for this id so enqueue() cannot be surprised later.
    m_reserved[base.id()] = Reservation{dl, wcet};
    return true;
}

void JobSporadicScheduler::enqueue(JobIDescriptor::Ptr base)
{
    if (m_stopped.load(std::memory_order_relaxed))
        return;

    auto spor = std::dynamic_pointer_cast<JobSporadicDescriptor>(base);
    if (!spor) {
        JOB_LOG_WARN("[Sporadic] Non-sporadic descriptor enqueued to sporadic scheduler; dropping (id={}).",
                     base ? base->id() : 0);
        return;
    }

    const uint64_t id = spor->id();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // If we have a reservation from admit(), consume it and push.
        if (auto it = m_reserved.find(id); it != m_reserved.end()) {
            // sanity:
            if (it->second.deadline != spor->deadline() || it->second.wcet != spor->wcet())
                JOB_LOG_WARN("[JobSporadicScheduler] sanity test failed in enqueue");

            m_reserved.erase(it);
            m_queue.push(std::move(spor));
        } else {
            // Ad-hoc path: caller skipped admit(). Check now (safe but can reject).
            const auto now = std::chrono::steady_clock::now();
            if (!canAdmitUnlocked(now, spor->deadline(), spor->wcet())) {
                JOB_LOG_WARN("[Sporadic] Task {} rejected at enqueue (no prior reservation).", id);
                return;
            }
            m_queue.push(std::move(spor));
        }
    }

    m_condition.notify_one();
}


std::optional<uint64_t> JobSporadicScheduler::next(uint32_t /*wid*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this]{
        return !m_queue.empty() || m_stopped.load(std::memory_order_relaxed);
    });

    if (m_queue.empty())
        return std::nullopt;

    // Earliest-deadline first; track as in-flight.
    auto desc = m_queue.top();
    m_queue.pop();

    const auto id = desc->id();
    m_inflight.emplace(id, std::move(desc));
    return id;
}


void JobSporadicScheduler::complete(uint64_t tid, bool /*ok*/)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_inflight.erase(tid);
    m_condition.notify_all();
}

size_t JobSporadicScheduler::size() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // keep existing semantics: "pending" only
    return m_queue.size();
}

void JobSporadicScheduler::stop()
{
    ISchedPolicy::stop();
    m_condition.notify_all();
}

// EDF:   O(n) + O(n log n) + O(n log n) + O(n) = O(n log n).
bool JobSporadicScheduler::canAdmitUnlocked(std::chrono::steady_clock::time_point now,
                                            std::chrono::steady_clock::time_point new_deadline,
                                            std::chrono::microseconds new_wcet) const
{
    if (new_wcet.count() <= 0)
        return true;

    if (new_deadline <= now)
        return false;

    struct Ticket {
        std::chrono::steady_clock::time_point dl;
        int64_t cost_us;
    };

    std::vector<Ticket> items;
    items.reserve(m_queue.size() + m_inflight.size() + m_reserved.size() + 1);

    // PARTY planing .... Pending
    auto qcopy = m_queue;
    while (!qcopy.empty()) {
        const auto& sp = qcopy.top();
        const auto cost = sp->wcet().count();
        if (cost > 0) items.push_back({ sp->deadline(), cost });
        qcopy.pop();
    }

    // In flight to the PARTY
    for (const auto &kv : m_inflight) {
        const auto &sp = kv.second;
        const auto cost = sp->wcet().count();
        if (cost > 0)
            items.push_back({ sp->deadline(), cost });
    }

    // hand out tickets to the PARTY
    for (const auto& kv : m_reserved) {
        const auto& r = kv.second;
        if (r.wcet.count() > 0)
            items.push_back({ r.deadline, r.wcet.count() });
    }

    // Welcome to the PARTY !!!!!
    items.push_back({ new_deadline, new_wcet.count() });

    std::sort(items.begin(), items.end(), [](const Ticket& a, const Ticket& b){
        return a.dl < b.dl;
    });

    int64_t demand_us = 0;
    // const int64_t cap = static_cast<int64_t>(m_capacity);
    const int64_t cap = std::max<int64_t>(1, static_cast<int64_t>(m_capacity));
    for (const auto& it : items) {
        demand_us += it.cost_us;

        const auto avail_us =
            std::chrono::duration_cast<std::chrono::microseconds>(it.dl - now).count();
        // you're late to the PARTY
        if (avail_us < 0)
            return false;

        // The PARTY spot is too small (cpu's)
        if (demand_us > cap * avail_us)
            return false;
    }

    return true;
}

} // namespace job::threads
// CHECKPOINT v1.1
