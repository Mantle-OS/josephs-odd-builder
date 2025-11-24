#pragma once

#include <atomic>
#include <memory>

#include "job_mcmp_queue.h"
#include "job_thread_pool.h"

namespace job::threads {

template <typename Msg>
class JobThreadActor : public std::enable_shared_from_this<JobThreadActor<Msg>> {
public:
    using Ptr = std::shared_ptr<JobThreadActor<Msg>>;
    explicit JobThreadActor(ThreadPool::Ptr pool, size_t mailboxCap = 1024) :
        m_pool{std::move(pool)},
        m_inbox{mailboxCap}
    {
    }

    virtual ~JobThreadActor()
    {
        stop();
    }

    void post(Msg msg, int priority = 0)
    {
        if(!m_pool)
            return;

        if (!m_inbox.push(std::move(msg)))
            return;

        schedule(priority);
    }

    void stop()
    {
        m_inbox.close();
    }

protected:
    virtual void onMessage(Msg &&msg) = 0;

private:
    void schedule(int priority) {
        if (!m_isScheduled.test_and_set(std::memory_order_acquire)) {
            auto self = this->shared_from_this();
            m_pool->submit(priority, [this, self, priority]() {
                this->processBatch(priority);
            });
        }
    }

    void processBatch(int priority)
    {
        int count = 0;
        Msg m;
        constexpr int kActorBatchSize = 16;
        while (count < kActorBatchSize) {
            if (m_inbox.tryPop(m)) {
                onMessage(std::move(m));
                count++;
            } else {
                m_isScheduled.clear(std::memory_order_release);
                if (!m_inbox.isEmpty()) {
                    if (!m_isScheduled.test_and_set(std::memory_order_acquire)) {
                        count = 0;
                        continue;
                    }
                }
                return;
            }
        }
        auto self = this->shared_from_this();
        m_pool->submit(priority, [this, self, priority]() {
            this->processBatch(priority);
        });
    }

    ThreadPool::Ptr              m_pool;
    JobBoundedMPMCQueue<Msg>     m_inbox;
    std::atomic_flag             m_isScheduled{0};
};

} // namespace job::threads
// CHECKPOINT: v1.1
