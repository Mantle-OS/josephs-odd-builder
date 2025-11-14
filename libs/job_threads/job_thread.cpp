#include "job_thread.h"

#include "job_thread_args.h"

namespace job::threads {

using JobThreadArgs = ThreadArgs<JobThread, JobThread::StartResult>;

JobThread::JobThread(const JobThreadOptions &options) noexcept :
    m_options{options}
{
}

JobThread::~JobThread() noexcept
{
    requestStop();
    (void)join();
}

void JobThread::setOptions(const JobThreadOptions &options) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_options = options;
}

void JobThread::setRunFunction(RunFunction fn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_runFunc = std::move(fn);
}

JobThread::StartResult JobThread::start()
{
    if (m_running.load())
        return StartResult::AlreadyRunning;

    auto promise = std::make_shared<std::promise<StartResult>>();
    auto future  = promise->get_future();
    auto *args = new (std::nothrow)JobThreadArgs{
        this,
        promise,
        m_stopSource.get_token()
    };

    if (!args)
        return StartResult::ThreadError;

    int create_result = pthread_create(&m_pthread,
                                       nullptr,
                                       &JobThread::threadEntry,
                                       args);

    if (create_result != 0) {
        delete args;
        m_joinable = false;
        promise->set_value(StartResult::ThreadError);
    } else {
        m_joinable = true;
    }

    return future.get();
}

void *JobThread::threadEntry(void* arg)
{
    std::unique_ptr<JobThreadArgs> args(static_cast<JobThreadArgs*>(arg));

    auto self    = args->self;
    auto promise = args->promise;
    auto token   = args->token;

    StartResult ret = StartResult::Started;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        int ok_sched = self->applyScheduling();
        int ok_aff   = self->applyAffinity();
        if (ok_sched != 0)
            ret = StartResult::SchedulingFailed;
        else if (ok_aff != 0)
            ret = StartResult::AffinityFailed;
        else
            ret = StartResult::Started;
    }

    promise->set_value(ret);

    if (ret == StartResult::Started) {
        self->m_running.store(true);
        // Set thread name
        {
            std::lock_guard<std::mutex> lock(self->m_mutex);
            if (self->m_options.name[0] != '\0') {
                self->m_options.name[self->m_options.name.size() - 1] = '\0';
                ::pthread_setname_np(::pthread_self(), self->m_options.name.data());
            }
        }
        RunFunction func_to_run;
        {
            std::lock_guard<std::mutex> lock(self->m_mutex);
            func_to_run = self->m_runFunc;
        }
        if (func_to_run)
            func_to_run(token);
        else
            self->run(token); // run() must also be thread-safe

        self->m_running.store(false);

    }
    return nullptr;
}

void JobThread::requestStop() noexcept
{
    m_stopSource.request_stop();
}

bool JobThread::join() noexcept
{
    if (m_joinable) {
        pthread_join(m_pthread, nullptr);
        m_joinable = false;
        return true;
    }
    return false;
}

bool JobThread::isRunning() const noexcept
{
    return m_running.load();
}

void JobThread::run(std::stop_token token) noexcept
{
    uint16_t heartbeat;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        heartbeat = m_options.heartbeat;
    }
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat));
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            heartbeat = m_options.heartbeat;
        }
    }
}

// NOTE: m_mutex MUST be locked
int JobThread::applyScheduling() noexcept
{
    if (m_options.realtime) {
        struct sched_param sched{};
        sched.sched_priority = m_options.priority;

        if (m_options.lockMemory) {
            if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
                perror("mlockall failed");
                return errno;
            }
        }

        if (pthread_setschedparam(pthread_self(),
                                  static_cast<int>(m_options.policy),
                                  &sched) != 0) {
            perror("pthread_setschedparam failed");
            return errno;
        }
    }

    return 0;
}
// NOTE: m_mutex MUST be locked
int JobThread::applyAffinity() noexcept
{
    if (m_options.pinToCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(m_options.coreId, &cpuset);

        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("sched_setaffinity failed");
            return errno;
        }
    }

    return 0;
}

} // job::threads

// CHECKPOINT: v1.6
