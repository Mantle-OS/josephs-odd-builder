#pragma once

#include "sched/job_work_stealing_scheduler.h"
#include "job_thread_pool.h"

namespace job::threads {

struct JobStealerCtx {
    JobWorkStealingScheduler::Ptr   scheduler;
    ThreadPool::Ptr                 pool;
    int                             threadCount{0};

    explicit JobStealerCtx(int threads = 0)
    {
        if (threads <= 0)
            threads = 4;

        threadCount = threads;
        scheduler   = std::make_shared<JobWorkStealingScheduler>(threads);
        pool        = ThreadPool::create(scheduler, threads);
    }

    ~JobStealerCtx()
    {
        if (pool)
            pool->shutdown();
    }

    JobStealerCtx(const JobStealerCtx&)            = delete;
    JobStealerCtx& operator=(const JobStealerCtx&) = delete;
    JobStealerCtx(JobStealerCtx&&)                 = default;
    JobStealerCtx& operator=(JobStealerCtx&&)      = default;
};

} // namespace job::threads
