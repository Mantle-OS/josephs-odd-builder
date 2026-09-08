#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>

#include "job_thread_options.h"
#include "jobthreads_export.h"

namespace job::threads {

class JOBTHREADS_EXPORT JobThread {
public:
    using RunFunction = std::function<void(std::stop_token)>;
    using Ptr = std::shared_ptr<JobThread>;

    enum class StartResult : std::uint8_t {
        Started,
        AlreadyRunning,
        SchedulingFailed,
        AffinityFailed,
        ThreadError
    };

    JobThread() noexcept = default;

    explicit JobThread(const JobThreadOptions &options) noexcept;
    virtual ~JobThread() noexcept;

    JobThread(const JobThread &) = delete;
    JobThread &operator=(const JobThread &) = delete;
    JobThread(JobThread &&) = delete;
    JobThread &operator=(JobThread &&) = delete;

    void setOptions(const JobThreadOptions &options) noexcept;

    void setRunFunction(RunFunction fn);

    void requestStop() noexcept;

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] bool isCurrentThread() const noexcept;
    [[nodiscard]] StartResult start();
    [[nodiscard]] bool join() noexcept;

protected:
    virtual void run(std::stop_token token) noexcept;

    // IN IMPL FILE (job_thread_<os>.cpp)
    [[nodiscard]] int applyScheduling() noexcept;
    [[nodiscard]] int applyAffinity() noexcept;

private:
    static void *threadEntry(void *arg);

    static constexpr std::size_t kHandleStorageSize = 16;

    mutable std::mutex m_mutex;
    JobThreadOptions m_options;

    std::atomic<bool> m_running{false};
    RunFunction m_runFunc;

    alignas(alignof(std::max_align_t)) unsigned char m_handleStorage[kHandleStorageSize]{};

    std::stop_source m_stopSource;

    bool m_joinable{false};
    std::atomic_flag m_joining{};
    std::atomic<int> m_lastJoinError{0};

    std::atomic_flag m_starting{};
};

} // namespace job::threads