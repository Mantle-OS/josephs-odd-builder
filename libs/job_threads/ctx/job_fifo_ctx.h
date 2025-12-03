#pragma once

#include "sched/job_fifo_scheduler.h"
#include "job_thread_pool.h"

namespace job::threads {

struct JobFifoCtx {
    FifoScheduler::Ptr  scheduler;
    ThreadPool::Ptr     pool;
    int                 threadCount{0};

    explicit JobFifoCtx(int threads = 0)
    {
        if (threads <= 0)
            threads = 4;

        threadCount = threads;
        scheduler   = std::make_shared<FifoScheduler>(threads);
        pool        = ThreadPool::create(scheduler, threads);
    }

    ~JobFifoCtx()
    {
        if (pool)
            pool->shutdown();
    }

    JobFifoCtx(const JobFifoCtx&)            = delete;
    JobFifoCtx &operator=(const JobFifoCtx&) = delete;
    JobFifoCtx(JobFifoCtx&&)                 = default;
    JobFifoCtx &operator=(JobFifoCtx&&)      = default;
};

} // namespace job::threads
