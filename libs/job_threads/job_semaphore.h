#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include <job_logger.h>


#include "jobthreads_export.h"

namespace job::threads {

enum class JobSemRet : std::uint8_t {
    OK          = 0,
    NotReady    = 1,
    Invalid     = 2,
    Timeout     = 3,
    Interrupted = 4,
    WouldBlock  = 5,
    Exists      = 6,
    NotFound    = 7,
    NoMemory    = 8,
    Permission  = 9,
    Unknown     = 255
};

[[nodiscard]] inline const char *semiRetToString(JobSemRet r)
{
    switch (r) {
    case JobSemRet::OK:          return "OK";
    case JobSemRet::NotReady:    return "NotReady";
    case JobSemRet::Invalid:     return "Invalid";
    case JobSemRet::Timeout:     return "Timeout";
    case JobSemRet::Interrupted: return "Interrupted";
    case JobSemRet::WouldBlock:  return "WouldBlock";
    case JobSemRet::Exists:      return "Exists";
    case JobSemRet::NotFound:    return "NotFound";
    case JobSemRet::NoMemory:    return "NoMemory";
    case JobSemRet::Permission:  return "Permission";
    case JobSemRet::Unknown:     return "Unknown";
    }
    return "Unknown";
}

enum class JobSemFlags : std::uint8_t {
    None            = 0,
    Create          = 1 << 0,  // O_CREAT (posix) / CreateSemaphoreW path (windows)
    Exclusive       = 1 << 1,  // O_EXCL (posix) / fail-if-ERROR_ALREADY_EXISTS (windows)
    UnlinkOnDestroy = 1 << 2   // when cleaning up call unlink
};

constexpr JobSemFlags operator|(JobSemFlags a, JobSemFlags b)
{
    return static_cast<JobSemFlags>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

constexpr JobSemFlags operator&(JobSemFlags a, JobSemFlags b)
{
    return static_cast<JobSemFlags>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

constexpr JobSemFlags& operator|=(JobSemFlags &a, JobSemFlags b)
{
    a = a | b;
    return a;
}

[[nodiscard]] constexpr bool hasFlag(JobSemFlags val, JobSemFlags bit)
{
    return (val & bit) != JobSemFlags::None;
}

class JOBTHREADS_EXPORT JobSem {
public:
    using Clock         = std::chrono::steady_clock;
    using TimePoint     = Clock::time_point;
    using PointDuration = std::chrono::milliseconds;
    using ClockDuration = Clock::duration;

    JobSem() = default;
    explicit JobSem(unsigned value, bool autoInit = false);
    ~JobSem();
    JobSem(const JobSem&)            = delete;
    JobSem &operator=(const JobSem&) = delete;

    [[nodiscard]] JobSemRet init(unsigned value = 0, bool pshared = true);
    [[nodiscard]] JobSemRet destroy();

    void setName(std::string name) { m_name = std::move(name); }
    [[nodiscard]] std::string name() const { return m_name; }

    [[nodiscard]] JobSemRet open(JobSemFlags flags = JobSemFlags::None,
                                 unsigned mode = 0666,
                                 unsigned value = 0);
    [[nodiscard]] JobSemRet close();

    [[nodiscard]] JobSemRet unlink();
    [[nodiscard]] JobSemRet wait();
    [[nodiscard]] JobSemRet wait(const PointDuration &timeout);

    [[nodiscard]] JobSemRet wait(int value, const PointDuration &timeout)
    {
        if (value <= 0)
            return JobSemRet::Invalid;

        auto per = timeout / value;
        if (per.count() <= 0)
            per = PointDuration(1);

        for (int i = 0; i < value; ++i) {
            JobSemRet r = wait(per);
            if (r != JobSemRet::OK)
                return r;
        }
        return JobSemRet::OK;
    }

    [[nodiscard]] JobSemRet wait(const ClockDuration &till)
    {
        return wait(std::chrono::duration_cast<PointDuration>(till));
    }

    [[nodiscard]] JobSemRet post();

    [[nodiscard]] JobSemRet value(int &out) const;

    [[nodiscard]] bool ready() const noexcept { return m_ready && m_sem != nullptr; }

private:
    [[nodiscard]] bool sanity() const noexcept { return m_ready && m_sem != nullptr; }
    [[nodiscard]] static JobSemRet fromPlatformError(long e);

    static constexpr std::size_t kInPlaceStorageSize = 64;

    bool        m_linked{false};        // named semaphore opened via open()
    bool        m_ready{false};         // open and ready to use
    bool        m_initedInPlace{false}; // init() vs open()
    bool        m_autoUnlink{false};    // unlink on destroy() for named sems
    std::string m_name;                 // for named semaphores
    void *m_sem{nullptr};
    alignas(alignof(std::max_align_t)) unsigned char m_inPlaceStorage[kInPlaceStorageSize]{};
    std::atomic<int> m_shadowCount{0};
};

} // namespace job::threads
