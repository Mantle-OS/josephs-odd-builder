#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <atomic>

#include <job_logger.h>
#include "job_thread_actor.h"
#include "job_thread_pool.h"

namespace job::threads {

template <typename In, typename Out>
class JobPipelineStage : public JobThreadActor<In> {
public:
    using ProcessFunc = std::function<std::optional<Out>(In&&)>;
    using ErrorFunc   = std::function<void(const std::exception&, const In&)>;
    using NextBase    = JobThreadActor<Out>;
    using NextPtr     = std::shared_ptr<NextBase>;
    using InputType   = In;
    using OutputType  = Out;


    JobPipelineStage(ThreadPool::Ptr pool, ProcessFunc process, size_t mailboxCap = 1024) :
        JobThreadActor<In>(std::move(pool), mailboxCap), m_process(std::move(process))
    {

    }

    // Fan-Out "supported"
    void addNext(NextPtr next)
    {
        if (next)
            m_next.push_back(next);
    }

    void setErrorHandler(ErrorFunc onError)
    {
        m_onError = std::move(onError);
    }

    // Metrics
    size_t getProcessedCount() const
    {
        return m_processedCount.load(std::memory_order_relaxed);
    }
    size_t getErrorCount() const
    {
        return m_errorCount.load(std::memory_order_relaxed);
    }

protected:
    void onMessage(In &&msg) override
    {
        if (!m_process)
            return;

        // Copy for error-reporting BEFORE we move from msg
        std::optional<In> original;

        //  ?? requires In to be copyable only IF they install an error handler ??
        if (m_onError)
            original = msg;

        try {
            std::optional<Out> out = m_process(std::move(msg));
            m_processedCount.fetch_add(1, std::memory_order_relaxed);

            if (!out.has_value())
                return;

            for (const auto &weakNext : m_next) {
                if (auto next = weakNext.lock()) {
                    if (!next->post(*out, 0)) {
                        JOB_LOG_WARN("[JobPipelineStage] Downstream actor rejected message (mailbox closed or full?).");
                        // TODO track this. .
                    }
                }
            }

        } catch (const std::exception &e) {
            m_errorCount.fetch_add(1, std::memory_order_relaxed);
            if (m_onError && original.has_value())
                m_onError(e, *original);
            else
                JOB_LOG_ERROR("[JobPipelineStage] Exception: {}", e.what());
        } catch (...) {
            m_errorCount.fetch_add(1, std::memory_order_relaxed);
            JOB_LOG_ERROR("[JobPipelineStage] Unknown Exception");
        }
    }


private:
    ProcessFunc                                 m_process;
    ErrorFunc                                   m_onError;
    std::vector<std::weak_ptr<NextBase>>        m_next;
    std::atomic<size_t>                         m_processedCount{0};
    std::atomic<size_t>                         m_errorCount{0};
};

template <typename In, typename Out>
std::shared_ptr<JobPipelineStage<In, Out>>
make_pipeline_stage(ThreadPool::Ptr pool, typename JobPipelineStage<In, Out>::ProcessFunc fn, size_t cap = 1024)
{
    return std::make_shared<JobPipelineStage<In, Out>>(std::move(pool), std::move(fn), cap);
}

} // namespace job::threads
// CHECKPOINT: v1.0
