#include "job_thread.h"

namespace job::threads {

JobThread::JobThread(const JobThreadOptions &options) :
    m_options{options}
{
}

JobThread::~JobThread()
{
    requestStop();
    join();
}

void JobThread::setOptions(const JobThreadOptions &options)
{
    m_options = options;
}

void JobThread::setRunFunction(RunFunction fn)
{
    m_runFunc = std::move(fn);
}

JobThread::StartResult JobThread::start()
{
    if (m_running.load())
        return StartResult::AlreadyRunning;

    try {
        m_thread = std::jthread([self = shared_from_this()](std::stop_token token) {
            if (!self->applyScheduling())
                throw std::runtime_error("Scheduling failed");

            if (!self->applyAffinity())
                throw std::runtime_error("Affinity failed");

            self->m_running.store(true);

            if (!self->m_options.name.empty())
                ::pthread_setname_np(::pthread_self(), self->m_options.name.c_str());

            if (self->m_runFunc)
                self->m_runFunc(token);
            else
                self->run(token);

            self->m_running.store(false);
        });
    } catch (const std::system_error &) {
        return StartResult::ThreadError;
    } catch (const std::runtime_error &e) {

        if (std::string_view(e.what()) == "Scheduling failed")
            return StartResult::SchedulingFailed;

        if (std::string_view(e.what()) == "Affinity failed")
            return StartResult::AffinityFailed;

        return StartResult::ThreadError;
    }

    return StartResult::Started;
}

void JobThread::requestStop()
{
    if (m_thread.joinable())
        m_thread.request_stop();
}

void JobThread::join()
{
    if (m_thread.joinable())
        m_thread.join();
}

bool JobThread::isRunning() const
{
    return m_running.load();
}

void JobThread::run(std::stop_token token)
{
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(m_options.heartbeat));
    }
}

bool JobThread::applyScheduling()
{
    bool ret = true;

    if (m_options.realtime) {
        struct sched_param sched{};
        sched.sched_priority = m_options.priority;

        if (m_options.lockMemory) {
            if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
                perror("mlockall failed");
                ret = false;
            }
        }

        if (pthread_setschedparam(pthread_self(),
                                  static_cast<int>(m_options.policy),
                                  &sched) != 0) {
            perror("pthread_setschedparam failed");
            ret = false;
        }
    }

    return ret;
}

bool JobThread::applyAffinity()
{
    bool ret = true;

    if (m_options.pinToCore) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(m_options.coreId, &cpuset);

        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("sched_setaffinity failed");
            ret = false;
        }
    }

    return ret;
}

}
