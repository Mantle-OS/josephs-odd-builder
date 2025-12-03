#pragma once

#include "sched/job_sporadic_scheduler.h"
#include "job_thread_pool.h"

namespace job::threads {

struct JobSporadicCtx {
    JobSporadicScheduler::Ptr   scheduler;
    ThreadPool::Ptr             pool;
    int                         threadCount{0};

    explicit JobSporadicCtx(int threads = 0)
    {
        if (threads <= 0)
            threads = 4;

        threadCount = threads;
        scheduler   = std::make_shared<JobSporadicScheduler>(threads);
        pool        = ThreadPool::create(scheduler, threads);
    }

    ~JobSporadicCtx()
    {
        if (pool)
            pool->shutdown();
    }

    JobSporadicCtx(const JobSporadicCtx&)            = delete;
    JobSporadicCtx& operator=(const JobSporadicCtx&) = delete;
    JobSporadicCtx(JobSporadicCtx&&)                 = default;
    JobSporadicCtx& operator=(JobSporadicCtx&&)      = default;
};

} // namespace job::threads
