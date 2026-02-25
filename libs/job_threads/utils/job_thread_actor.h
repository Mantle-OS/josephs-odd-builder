#pragma once

#include <atomic>
#include <memory>

#include "queue/job_mcmp_queue.h"
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

    [[nodiscard]] bool post(Msg msg, int priority = 0)
    {
        if(!m_pool)
            return false;

        if (!m_inbox.push(std::move(msg)))
            return false;

        schedule(priority);
        return true;
    }

    void stop()
    {
        m_inbox.close();
    }

    [[nodiscard]] bool isAlive() const
    {
        if (!m_pool)
            return false;

        if (!m_inbox.isEmpty())
            return true;

        return m_isScheduled.load(std::memory_order_acquire);
    }


protected:
    virtual void onMessage(Msg &&msg) = 0;

private:
    void schedule(int priority) {
        if (!m_isScheduled.exchange(true, std::memory_order_acquire)) {
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
                m_isScheduled.store(false, std::memory_order_release);
                if (!m_inbox.isEmpty()) {
                    if (!m_isScheduled.exchange(true, std::memory_order_acquire)) {
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
    std::atomic<bool>            m_isScheduled{false};
};

} // namespace job::threads
