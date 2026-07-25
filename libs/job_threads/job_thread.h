#pragma once

#include <atomic>
#include <functional>
#include <stop_token>
#include <mutex>
#include <memory>
#include <cstdint>

#include "job_thread_options.h"
#include "jobthreads_export.h"

// NOTE ALL IMPL FILES MUST DECL THE FOLLOWING
// [[nodiscard]] StartResult start();
// [[nodiscard]] bool join() noexcept;
// [[nodiscard]] int applyScheduling() noexcept;
// [[nodiscard]] int applyAffinity() noexcept;
// static TYPE &provider(unsigned char *storage) noexcept { return *reinterpret_cast<TYPE*>(storage);}
// where stype is p_thread for an example. so you can call provider(m_handleStorage)

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

    explicit JobThread(const JobThreadOptions &options) noexcept :
        m_options(options)
    {

    }

    virtual ~JobThread() noexcept
    {
        requestStop();
        (void)join();

    }

    void setOptions(const JobThreadOptions &options) noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_options = options;
    }

    void setRunFunction(RunFunction fn)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_runFunc = std::move(fn);
    }

    void requestStop() noexcept
    {
        m_stopSource.request_stop();
    }

    [[nodiscard]] bool isRunning() const noexcept
    {
        return m_running.load(std::memory_order_relaxed);
    }


    // IN IMPL FILE
    [[nodiscard]] StartResult start();
    [[nodiscard]] bool join() noexcept;

protected:
    virtual void run(std::stop_token token) noexcept
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


    // IN IMPL FILE (in job_thread_<os>.cpp)
    [[nodiscard]] int applyScheduling() noexcept;
    [[nodiscard]] int applyAffinity() noexcept;

private:
#if defined(_WIN32)
    static unsigned __stdcall threadEntry(void *arg);
#else
    static void *threadEntry(void *arg);
#endif

    static constexpr std::size_t kHandleStorageSize = 16;

    mutable std::mutex                                  m_mutex;
    JobThreadOptions                                    m_options;
    std::atomic<bool>                                   m_running{false};
    RunFunction                                         m_runFunc;
    alignas(alignof(std::max_align_t)) unsigned char    m_handleStorage[kHandleStorageSize]{};
    std::stop_source                                    m_stopSource;

    bool                                                m_joinable{false};
    std::atomic_flag                                    m_joining{false};
    std::atomic<int>                                    m_lastJoinError{0};

    std::atomic_flag                                    m_starting{false};
};

} // namespace job::threads
