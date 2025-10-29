#pragma once

#include <memory>
#include <atomic>
#include <functional>
#include <thread>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>

#include "thread_options.h"

namespace job::threads {

// NOTES noexcept and other (moderen c++ 23)bits that could be added for compiler fun

class JobThread : public std::enable_shared_from_this<JobThread> {
public:
    enum class StartResult : uint8_t {
        Started,
        AlreadyRunning,
        SchedulingFailed,
        AffinityFailed,
        ThreadError
    };
    using RunFunction = std::function<void(std::stop_token)>;

    JobThread() = default;
    explicit JobThread(const JobThreadOptions &options);
    virtual ~JobThread();

    void setOptions(const JobThreadOptions &options);
    void setRunFunction(RunFunction fn);

    [[nodiscard]] StartResult start();
    void requestStop();

    void join();

    [[nodiscard]] bool isRunning() const;

protected:
    virtual void run(std::stop_token token);
    [[nodiscard]] bool applyScheduling();
    [[nodiscard]] bool applyAffinity();

private:
    JobThreadOptions    m_options;
    std::jthread        m_thread;
    std::atomic<bool>   m_running{false};
    RunFunction         m_runFunc;
};

} // namespace job::threads
