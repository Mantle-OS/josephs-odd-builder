#include "engine_threads.h"

#include "sched/job_fifo_scheduler.h"
#include "sched/job_round_robin_scheduler.h"
#include "sched/job_sporadic_scheduler.h"
#include "sched/job_work_stealing_scheduler.h"

namespace job::science::engine {

EngineThreads makeThreadSched(SchedulerType type, std::uint32_t workers)
{
    EngineThreads out{};

    if (workers == 0)
        workers = std::max(1u, std::uint32_t(std::thread::hardware_concurrency()));

    out.threadCount = workers;

    switch (type) {
    case SchedulerType::Fifo:
        out.scheduler = std::make_shared<FifoScheduler>(workers);
        break;
    case SchedulerType::RoundRobin:
        out.scheduler = std::make_shared<JobRoundRobinScheduler>(workers);
        break;
    case SchedulerType::Sporadic:
        out.scheduler = std::make_shared<JobSporadicScheduler>(workers);
        break;
    case SchedulerType::WorkStealing:
        out.scheduler = std::make_shared<JobWorkStealingScheduler>(workers);
        break;
    default:
        out.scheduler = std::make_shared<FifoScheduler>(workers);
        break;
    }

    out.pool = ThreadPool::create(out.scheduler, workers);
    return out;
}

}
