#pragma once

#include <atomic>
#include <functional>
#include <stop_token>
#include <unistd.h>
#include <mutex>
#include <memory>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>

#include "job_thread_options.h"

namespace job::threads {

class JobThread  {
public:
    using RunFunction = std::function<void(std::stop_token)>;
    using Ptr = std::shared_ptr<JobThread>;

    enum class StartResult : uint8_t {
        Started,
        AlreadyRunning,
        SchedulingFailed,
        AffinityFailed,
        ThreadError
    };
    JobThread() noexcept = default;
    explicit JobThread(const JobThreadOptions &options) noexcept;
    virtual ~JobThread() noexcept;

    void setOptions(const JobThreadOptions &options) noexcept;
    void setRunFunction(RunFunction fn);

    [[nodiscard]] StartResult start();
    void requestStop() noexcept;

    [[nodiscard]] bool join() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

protected:
    virtual void run(std::stop_token token) noexcept;
    [[nodiscard]] int applyScheduling() noexcept;
    [[nodiscard]] int applyAffinity() noexcept;

private:
    static void* threadEntry(void* arg);

    mutable std::mutex  m_mutex;
    JobThreadOptions    m_options;
    std::atomic<bool>   m_running{false};
    RunFunction         m_runFunc;
    pthread_t           m_pthread{};
    std::stop_source    m_stopSource;
    bool                m_joinable{false};
};

} // namespace job::threads
// CHECKPOINT: v1.6
