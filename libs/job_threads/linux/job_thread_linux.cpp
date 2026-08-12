#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>

#include <cerrno>
#include <cstring>
#include <future>
#include <memory>
#include <new>

#include "job_thread.h"
#include "job_thread_args.h"

namespace job::threads {

static pthread_t &provider(unsigned char *handleStorage) noexcept {return *reinterpret_cast<pthread_t *>(handleStorage);}
using JobThreadArgs = ThreadArgs<JobThread, JobThread::StartResult>;

JobThread::StartResult JobThread::start()
{
    // m_starting serializes attempts to transition this object into a new execution. If another start is already underway, reject this one.
    if (m_starting.test_and_set(std::memory_order_acq_rel))
        return StartResult::AlreadyRunning;

    // A running thread cannot be started again. A completed but still joinable thread must also be joined before m_handleStorage can safely be reused for a new pthread handle.
    if (m_running.load(std::memory_order_acquire) || m_joinable) {
        m_starting.clear(std::memory_order_release);
        return StartResult::AlreadyRunning;
    }

    // std::stop_source cannot be reset after request_stop(). Every new execution therefore requires a fresh stop state.
    m_stopSource = std::stop_source{};

    auto promise = std::make_shared<std::promise<StartResult>>();
    auto future  = promise->get_future();

    auto *args = new (std::nothrow) JobThreadArgs {
        this,
        promise,
        m_stopSource.get_token()
    };

    if (!args) {
        m_starting.clear(std::memory_order_release);
        return StartResult::ThreadError;
    }

    int const createResult = pthread_create(&provider(m_handleStorage), nullptr, &JobThread::threadEntry, args);

    if (createResult != 0) {
        delete args;

        m_joinable = false;
        m_starting.clear(std::memory_order_release);

        return StartResult::ThreadError;
    }

    m_joinable = true;

    StartResult const result = future.get();

    // If startup initialization failed, reap the created pthread immediately so the JobThread object returns to a reusable state.
    if (result != StartResult::Started && m_joinable) {
        (void)pthread_join(provider(m_handleStorage), nullptr);
        m_joinable = false;
    }

    return result;
}

void *JobThread::threadEntry(void *arg)
{
    std::unique_ptr<JobThreadArgs> args(static_cast<JobThreadArgs *>(arg));
    auto *self    = args->self;
    auto promise  = args->promise;
    auto token    = args->token;

    StartResult result = StartResult::Started;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);

        if (self->applyScheduling() != 0)
            result = StartResult::SchedulingFailed;

        if (result == StartResult::Started && self->m_options.pinToCore && self->applyAffinity() != 0)
            result = StartResult::AffinityFailed;
    }

    if (result == StartResult::Started)
        self->m_running.store(true, std::memory_order_release);

    // Unblock the caller waiting inside start().
    promise->set_value(result);

    if (result != StartResult::Started) {
        self->m_starting.clear(std::memory_order_release);
        return nullptr;
    }

    // Set the native thread name after startup has succeeded.
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        if (self->m_options.name[0] != '\0') {
            char nameBuffer[16]{};
            std::strncpy(nameBuffer, self->m_options.name.data(), sizeof(nameBuffer) - 1);
            (void)::pthread_setname_np(::pthread_self(), nameBuffer);
        }
    }

    RunFunction functionToRun;

    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        functionToRun = self->m_runFunc;
    }

    // Startup is now complete \0/. Further start() calls are rejected by m_running
    self->m_starting.clear(std::memory_order_release);

    if (functionToRun)
        functionToRun(token);
    else
        self->run(token);

    self->m_running.store(false, std::memory_order_release);
    return nullptr;
}

bool JobThread::join() noexcept
{
    if (m_joining.test_and_set(std::memory_order_acq_rel))
        return false;

    if (!m_joinable) {
        m_lastJoinError.store(0, std::memory_order_release);
        m_joining.clear(std::memory_order_release);
        return false;
    }

    int const result = pthread_join(provider(m_handleStorage), nullptr);

    if (result != 0) {
        // Preserve m_joinable. its not proven that the native thread was successfully reaped, so its storage must not be reused by start().
        m_lastJoinError.store(result, std::memory_order_release);
        m_joining.clear(std::memory_order_release);
        return false;
    }

    m_joinable = false;
    m_lastJoinError.store(0, std::memory_order_release);
    m_joining.clear(std::memory_order_release);
    return true;
}

// NOTE: m_mutex MUST be locked.
int JobThread::applyScheduling() noexcept
{
    if (!m_options.valid())
        return EINVAL;

    if (!m_options.realtime)
        return 0;

    if (m_options.lockMemory)
        if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
            return errno;

    sched_param parameters{};
    parameters.sched_priority = m_options.priority;

    int const policy = m_options.policy == SchedulingPolicy::FIFO ?
                           SCHED_FIFO :
                           SCHED_RR;

    // pthread_setschedparam returns the error number directly.
    int const result = pthread_setschedparam(pthread_self(), policy, &parameters);
    if (result != 0)
        return result;

    return 0;
}

// NOTE: m_mutex MUST be locked.
int JobThread::applyAffinity() noexcept
{
    if (!m_options.pinToCore)
        return 0;

    if (m_options.coreId == JobThreadOptions::kCoreUnbound)
        return EINVAL;

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(m_options.coreId, &cpuSet);

    if (sched_setaffinity( 0, sizeof(cpu_set_t), &cpuSet ) != 0)
        return errno;

    return 0;
}

} // namespace job::threads