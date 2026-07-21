#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <chrono>

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

    // "fix note": a naive shallow copy would leave the copy's handle
    // pointing at the original's in-place storage. Semaphores aren't a
    // meaningfully copyable resource anyway.
    JobSem(const JobSem&)            = delete;
    JobSem &operator=(const JobSem&) = delete;

    //////////////////////////////////
    // MEMORY (unnamed / in-place)
    //////////////////////////////////

    // pshared=true requests a POSIX process-shared, memory-embedded
    // semaphore. Windows has no equivalent (a Windows semaphore is always
    // an opaque per-process HANDLE, never memory-mappable bytes) -- see
    // job_semaphore_win32.cpp for how that's handled.
    [[nodiscard]] JobSemRet init(unsigned value = 0, bool pshared = true);
    [[nodiscard]] JobSemRet destroy();

    //////////////////////////////////
    // NAMED semaphores
    //////////////////////////////////

    void setName(std::string name) { m_name = std::move(name); }
    [[nodiscard]] std::string name() const { return m_name; }

    // Name must be POSIX-style (start with '/'). mode/value only used if
    // the Create flag is set. mode is a plain permission-bits integer
    // (POSIX mode_t on posix; unused/ignored on Windows, which has no
    // equivalent concept for named kernel objects here).
    [[nodiscard]] JobSemRet open(JobSemFlags flags = JobSemFlags::None,
                                 unsigned mode = 0666,
                                 unsigned value = 0);
    [[nodiscard]] JobSemRet close();

    // Windows named kernel objects are reference-counted and vanish once
    // the last handle closes -- there's no "detach the name while keeping
    // the object alive" primitive, but also no need for one: unlink()'s
    // intent (don't let this name outlive its last user) is already
    // exactly what Windows does automatically. Documented no-op there.
    [[nodiscard]] JobSemRet unlink();

    //////////////////////////////////
    // ACTIONS (wait / timed wait / post)
    //////////////////////////////////

    [[nodiscard]] JobSemRet wait();
    [[nodiscard]] JobSemRet wait(const PointDuration &timeout);

    // Portable: pure looping logic over the platform-specific wait(timeout)
    // above, no OS calls of its own -- kept inline rather than duplicated
    // in both backends.
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

    //////////////////////////////////
    // META Information
    //////////////////////////////////

    [[nodiscard]] JobSemRet value(int &out) const;

    [[nodiscard]] bool ready() const noexcept { return m_ready && m_sem != nullptr; }

private:
    [[nodiscard]] bool sanity() const noexcept { return m_ready && m_sem != nullptr; }

    // posix: maps errno. windows: maps GetLastError(). Same shape, backend
    // decides what "platform error" means.
    [[nodiscard]] static JobSemRet fromPlatformError(long e);

    static constexpr std::size_t kInPlaceStorageSize = 64;

    bool        m_linked{false};        // named semaphore opened via open()
    bool        m_ready{false};         // open and ready to use
    bool        m_initedInPlace{false}; // init() vs open()
    bool        m_autoUnlink{false};    // unlink on destroy() for named sems
    std::string m_name;                 // for named semaphores

    // The live handle: sem_t* on posix (may point into m_inPlaceStorage
    // for in-place mode, or into libc-managed memory from sem_open for
    // named mode), or a Windows semaphore HANDLE (HANDLE is `void*` under
    // the hood, so this needs no byte-buffer/reinterpret_cast trick the
    // way JobThread's pthread_t did).
    void *m_sem{nullptr};

    // Backing storage for the POSIX in-place (sem_init) semaphore. Present
    // but unused on Windows, where every mode is backed by a real kernel
    // semaphore object instead (see init()'s doc comment above).
    alignas(alignof(std::max_align_t)) unsigned char m_inPlaceStorage[kInPlaceStorageSize]{};

    // Windows has no public API to read a semaphore's current count
    // without a side effect (unlike POSIX sem_getvalue). Updated in the
    // same call as every post()/wait() on the Windows backend; unused on
    // posix, which has a real sem_getvalue. Present on both platforms for
    // layout simplicity, same treatment as m_inPlaceStorage above.
    std::atomic<int> m_shadowCount{0};
};

} // namespace job::threads
