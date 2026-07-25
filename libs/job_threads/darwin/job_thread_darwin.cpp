#include "job_thread.h"

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <cerrno>
#include <cstring>

#include "job_thread_args.h"

namespace job::threads {

static pthread_t &nativeHandle(unsigned char *storage) noexcept
{
    return *reinterpret_cast<pthread_t*>(storage);
}

static int toNativePolicy(SchedulingPolicy policy) noexcept
{
    switch (policy) {
    case SchedulingPolicy::FIFO:       return SCHED_FIFO;
    case SchedulingPolicy::RoundRobin: return SCHED_RR;
    case SchedulingPolicy::Other:
    default:                           return SCHED_OTHER;
    }
}

using JobThreadArgs = ThreadArgs<JobThread, JobThread::StartResult>;

JobThread::StartResult JobThread::start()
{
    if (m_running.load(std::memory_order_acquire) || m_starting.test_and_set(std::memory_order_acq_rel))
        return StartResult::AlreadyRunning;

    auto promise = std::make_shared<std::promise<StartResult>>();
    auto future  = promise->get_future();
    auto *args = new (std::nothrow) JobThreadArgs{
        this,
        promise,
        m_stopSource.get_token()
    };

    if (!args) {
        m_starting.clear(std::memory_order_release);
        return StartResult::ThreadError;
    }

    int create_result = pthread_create(&nativeHandle(m_handleStorage),
                                       nullptr,
                                       &JobThread::threadEntry,
                                       args);

    if (create_result != 0) {
        delete args;
        m_joinable = false;
        promise->set_value(StartResult::ThreadError);
        m_starting.clear(std::memory_order_release);
    } else {
        m_joinable = true;
    }

    StartResult result = future.get();

    if (result != StartResult::Started && m_joinable) {
        pthread_join(nativeHandle(m_handleStorage), nullptr);
        m_joinable = false;
    }

    return result;
}

void *JobThread::threadEntry(void *arg)
{
    std::unique_ptr<JobThreadArgs> args(static_cast<JobThreadArgs*>(arg));
    auto self = args->self;
    auto promise = args->promise;
    auto token = args->token;

    StartResult ret = StartResult::Started;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        if (self->m_options.realtime) {
            if (self->applyScheduling() != 0) {
                ret = StartResult::SchedulingFailed;
            }
        }
        if (ret == StartResult::Started && self->m_options.pinToCore) {
            if (self->applyAffinity() != 0) {
                ret = StartResult::AffinityFailed;
            }
        }
    }

    promise->set_value(ret);

    if (ret != StartResult::Started) {
        self->m_starting.clear(std::memory_order_release);
        return nullptr;
    }

    self->m_running.store(true, std::memory_order_release);
    self->m_starting.clear(std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        if (self->m_options.name[0] != '\0') {
            char nameBuf[16]{};
            std::strncpy(nameBuf, self->m_options.name.data(), sizeof(nameBuf) - 1);
            ::pthread_setname_np(::pthread_self(), nameBuf);
        }
    }

    RunFunction func_to_run;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        func_to_run = self->m_runFunc;
    }

    if (func_to_run) {
        func_to_run(token);
    } else {
        self->run(token);
    }

    self->m_running.store(false, std::memory_order_release);
    return nullptr;
}

int JobThread::applyScheduling() noexcept
{
    if (m_options.realtime) {
        struct sched_param sched{};
        sched.sched_priority = m_options.priority;

        if (m_options.lockMemory) {
            if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
                return errno;
            }
        }

        if (pthread_setschedparam(pthread_self(),
                                  toNativePolicy(m_options.policy),
                                  &sched) != 0) {
            return errno;
        }
    }

    return 0;
}

int JobThread::applyAffinity() noexcept
{
    if (m_options.pinToCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(m_options.coreId, &cpuset);

        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
            return errno;
        }
    }

    return 0;
}

} // namespace job::threads