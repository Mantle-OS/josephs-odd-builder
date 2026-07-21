#pragma once
#include <atomic>
#include <functional>
#include <stop_token>
#include <mutex>
#include <memory>
#include <cstdint>

#include "job_thread_options.h"
#include "jobthreads_export.h"

namespace job::threads {

class JOBTHREADS_EXPORT JobThread {
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
#if defined(_WIN32)
    static unsigned __stdcall threadEntry(void *arg);
#else
    static void *threadEntry(void *arg);
#endif

    // Opaque storage for the platform's native thread handle
    // (pthread_t on posix, HANDLE on Windows). Sized/aligned generously
    // for either; the real type is only ever named inside the platform
    // backend .cpp that implements these member functions.
    static constexpr std::size_t kHandleStorageSize = 16;

    mutable std::mutex  m_mutex;
    JobThreadOptions    m_options;
    std::atomic<bool>   m_running{false};
    RunFunction         m_runFunc;
    alignas(alignof(std::max_align_t)) unsigned char m_handleStorage[kHandleStorageSize]{};
    std::stop_source    m_stopSource;
    bool                m_joinable{false};
    std::atomic_flag    m_starting{false};
};

} // namespace job::threads