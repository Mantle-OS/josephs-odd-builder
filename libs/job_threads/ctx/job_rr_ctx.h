#pragma once

#include "sched/job_round_robin_scheduler.h"
#include "job_thread_pool.h"

namespace job::threads {

struct JobRoundRobinCtx {
    JobRoundRobinScheduler::Ptr scheduler;
    ThreadPool::Ptr             pool;
    int                         threadCount{0};

    explicit JobRoundRobinCtx(int threads = 0)
    {
        if (threads <= 0)
            threads = 4;

        threadCount = threads;
        scheduler   = std::make_shared<JobRoundRobinScheduler>(threads);
        pool        = ThreadPool::create(scheduler, threads);
    }

    ~JobRoundRobinCtx()
    {
        if (pool)
            pool->shutdown();
    }

    JobRoundRobinCtx(const JobRoundRobinCtx&)            = delete;
    JobRoundRobinCtx& operator=(const JobRoundRobinCtx&) = delete;
    JobRoundRobinCtx(JobRoundRobinCtx&&)                 = default;
    JobRoundRobinCtx& operator=(JobRoundRobinCtx&&)      = default;
};

} // namespace job::threads
