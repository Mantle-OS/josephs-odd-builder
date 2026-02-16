#pragma once
#include <cstdint>

#include <job_thread_pool.h>

#include <sched/job_isched_policy.h>
#include <sched/job_scheduler_types.h>

#include <sched/job_fifo_scheduler.h>
#include <sched/job_round_robin_scheduler.h>
#include <sched/job_sporadic_scheduler.h>
#include <sched/job_work_stealing_scheduler.h>

namespace job::science::engine {

using namespace job::threads;

struct EngineThreads {
    ISchedPolicy::Ptr           scheduler;
    SchedulerType               schedulerType;

    ThreadPool::Ptr             pool;
    std::uint32_t               threadCount{0};

    using Ptr = std::shared_ptr<EngineThreads>;
    explicit EngineThreads(SchedulerType sched = threads::SchedulerType::Fifo, int threads = 0)
    {
        update(sched, threads);
    }
    ~EngineThreads()
    {
        shutdown();
    }
    void shutdown()
    {
        if (pool)
            pool->shutdown();
        pool.reset();
        scheduler.reset();
    }

    void update(SchedulerType type, std::uint32_t workers)
    {
        shutdown();
        if (workers == 0)
            workers = std::max(1u, std::uint32_t(std::thread::hardware_concurrency()));

        this->threadCount = workers;

        // if this gets updated
        if(scheduler)
            scheduler->stop();

        switch (type) {
        case SchedulerType::Fifo:
            scheduler = std::make_shared<FifoScheduler>(workers);
            break;
        case SchedulerType::RoundRobin:
            scheduler   = std::make_shared<JobRoundRobinScheduler>(workers);
            break;
        case SchedulerType::Sporadic:
            scheduler = std::make_shared<JobSporadicScheduler>(workers);
            break;
        case SchedulerType::WorkStealing:
            scheduler = std::make_shared<JobWorkStealingScheduler>(workers);
            break;
        default:
            scheduler = std::make_shared<FifoScheduler>(workers);
            break;
        }
        schedulerType = type;

        pool = ThreadPool::create(scheduler, workers);


    }

    EngineThreads(const EngineThreads&)            = delete;
    EngineThreads& operator=(const EngineThreads&) = delete;
    EngineThreads(EngineThreads&&)                 = default;
    EngineThreads& operator=(EngineThreads&&)      = default;
};

[[nodiscard]] EngineThreads makeThreadSched(SchedulerType type, std::uint32_t workers);

}
