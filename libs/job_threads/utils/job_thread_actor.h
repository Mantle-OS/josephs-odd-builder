#pragma once

#include "job_mcmp_queue.h"
#include "job_task_queue.h"
#include "job_thread_pool.h"

namespace job::threads {
template <typename Msg>
class JobThreadActor {
public:
    explicit JobThreadActor(ThreadPool::Ptr pool, int priority = 0) :
        m_pool{pool},
        m_future{m_pool->submit(priority, [this]{this->loop();})}
    {}
    virtual ~JobThreadActor() { stop(); }

    void post(Msg msg)
    {
        m_inbox.push(std::move(msg));
    }
    void stop() {
        if (m_running.exchange(false)) {
            m_inbox.close(); // wake up sleepy head
            m_future.wait();
        }
    }

protected:
    virtual void onMessage(Msg&&) = 0;

private:
    void loop() {
        Msg m;
        while (m_running.load()) {
            if (m_inbox.pop(m))
                on_message(std::move(m));
            else
                break;
        }
    }

    ThreadPool::Ptr             m_pool;
    JobBoundedMPMCQueue<Msg>    m_inbox{1024};
    std::atomic<bool>           m_running{false};
    std::future<void>           m_future;
};

} // namespace job::threads
// CHECKPOINT: v1.0


