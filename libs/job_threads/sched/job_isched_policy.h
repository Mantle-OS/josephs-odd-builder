#pragma once

#include <optional>
#include <memory>
#include <atomic>

#include "descr/job_idescriptor.h"

namespace job::threads {
class ISchedPolicy {
public:
    using Ptr  = std::shared_ptr<ISchedPolicy>;
    using WPtr = std::weak_ptr<ISchedPolicy>;
    virtual ~ISchedPolicy () = default;

    virtual JobIDescriptor::Ptr createDescriptor(uint64_t id, int priority) = 0;

    // Called when a task is created or its metadata changes.
    virtual void enqueue(JobIDescriptor::Ptr desc) = 0;

    // Called when a worker becomes idle and asks for the next task.
    // Returns the id of the task to run next (if any).
    [[nodiscard]] virtual std::optional<uint64_t> next(uint32_t wid) = 0;

    // success/fail so the policy can learn.
    virtual void complete(uint64_t tid, bool ok) = 0;

    // per-host/domain concurrency limiting hook
    [[nodiscard]] virtual bool admit([[maybe_unused]] uint32_t wid,
                                     [[maybe_unused]] const JobIDescriptor &desc)
    {
        return true;
    }

    // ThreadWatcher integration
    [[nodiscard]] virtual size_t size() const noexcept = 0;

    // allows ThreadPool to wake up a sleepy head sched
    virtual void stop()
    {
        m_stopped.store(true, std::memory_order_relaxed);
    }

    // is the thread on the sched running
    [[nodiscard]] bool stopped() const noexcept
    {
        return m_stopped.load(std::memory_order_relaxed);
    }

protected:
    std::atomic<bool>   m_stopped{false};

};

} // namespace job::threads
// CHECKPOINT: v1.0
