#pragma once
#include <functional>
#include <exception>

#include "job_thread_pool.h"
#include "utils/job_thread_actor.h"

namespace job::threads {

// messages at the end of the "line".
template <typename In>
class JobPipelineSink : public JobThreadActor<In> {
public:
    using InputType     = In;
    using OutputType    = void;
    using ConsumeFunc   = std::function<void(In&&)>;

    JobPipelineSink(ThreadPool::Ptr pool, ConsumeFunc consume, size_t mailboxCap = 1024) :
        JobThreadActor<In>(std::move(pool), mailboxCap),
        m_consume(std::move(consume))
    {

    }

protected:
    void onMessage(In &&msg) override
    {
        if (m_consume) {
            try {
                m_consume(std::move(msg));
            } catch (const std::exception &e) {
                JOB_LOG_ERROR("[JobPipelineSink] Exception: {}", e.what());
            }
        }
    }

private:
    ConsumeFunc m_consume;
};

template <typename In>
std::shared_ptr<JobPipelineSink<In>>
make_pipeline_sink(ThreadPool::Ptr pool, typename JobPipelineSink<In>::ConsumeFunc fn, size_t cap = 1024)
{
    return std::make_shared<JobPipelineSink<In>>(std::move(pool), std::move(fn), cap);
}

} // namespace job::threads
// CHECKPOINT: v1.0
