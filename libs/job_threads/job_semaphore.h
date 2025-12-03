#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <semaphore.h>
#include <string>
#include <chrono>
#include <ctime>
#include <fcntl.h>

#include <job_logger.h>

namespace job::threads {

// result codes for functions/methods
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
    // Rerserved for more later if needed
    Unknown     = 255
};

// debugging the return value in other parts of trhe code
[[nodiscard]] inline const char *semiRetToString(JobSemRet r)
{
    switch (r) {
    case JobSemRet::OK:
        return "OK";
    case JobSemRet::NotReady:
        return "NotReady";
    case JobSemRet::Invalid:
        return "Invalid";
    case JobSemRet::Timeout:
        return "Timeout";
    case JobSemRet::Interrupted:
        return "Interrupted";
    case JobSemRet::WouldBlock:
        return "WouldBlock";
    case JobSemRet::Exists:
        return "Exists";
    case JobSemRet::NotFound:
        return "NotFound";
    case JobSemRet::NoMemory:
        return "NoMemory";
    case JobSemRet::Permission:
        return "Permission";
    case JobSemRet::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

// open flags
enum class JobSemFlags : std::uint8_t {
    None            = 0,
    Create          = 1 << 0,  // O_CREAT
    Exclusive       = 1 << 1,  // O_EXCL
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

class JobSem {
public:
    using Clock         = std::chrono::steady_clock;
    using TimePoint     = Clock::time_point;
    using PointDuration = std::chrono::milliseconds;
    using ClockDuration = Clock::duration;

    JobSem() = default;

    explicit JobSem(unsigned value, bool autoInit = false)
    {
        if (autoInit)
            if (init(value) != JobSemRet::OK)
                JOB_LOG_WARN("[JobSem] Failed to auto init the semaphore");
    }

    ~JobSem()
    {
        // don't throw.
        if (!m_sem)
            return;

        if (!m_linked && m_initedInPlace) {
            if (::sem_destroy(m_sem) != 0)
                JOB_LOG_WARN("[JobSem] sem_destroy in destructor failed: {}", std::strerror(errno));
        } else {
            if (::sem_close(m_sem) != 0)
                JOB_LOG_WARN("[JobSem] sem_close in destructor failed: {}", std::strerror(errno));
        }
    }

    //////////////////////////////////
    // MEMORY (unnamed / in-place)
    //////////////////////////////////

    // Initialize an unnamed semaphore in-place (for shared memory or local use)
    [[nodiscard]] JobSemRet init(unsigned value = 0, bool pshared = true)
    {
        if (m_sem) {
            JOB_LOG_WARN("[JobSem] init() called on already-initialized semaphore");
            return JobSemRet::Invalid;
        }

        int rc = ::sem_init(&m_storage, pshared ? 1 : 0, value);
        if (rc == 0) {
            m_sem           = &m_storage;
            m_ready         = true;
            m_linked        = false;
            m_initedInPlace = true;
            return JobSemRet::OK;
        }
        return fromErrno(errno);
    }

    // close + unlink iof needed.
    [[nodiscard]] JobSemRet destroy()
    {
        if (!m_sem)
            return JobSemRet::NotReady;

        JobSemRet result = JobSemRet::OK;
        if (!m_linked && m_initedInPlace) {
            if (::sem_destroy(m_sem) != 0)
                result = fromErrno(errno);
        } else {
            // named semaphore: close it, optionally unlink
            if (::sem_close(m_sem) != 0)
                result = fromErrno(errno);

            if (m_autoUnlink && !m_name.empty()) {
                if (::sem_unlink(m_name.c_str()) != 0) {
                    // If previous result was OK, update; otherwise keep first failure.
                    if (result == JobSemRet::OK)
                        result = fromErrno(errno);
                }
            }
        }

        m_sem           = nullptr;
        m_ready         = false;
        m_initedInPlace = false;
        m_linked        = false;

        return result;
    }

    //////////////////////////////////
    // NAMED semaphores
    //////////////////////////////////

    // Name must be POSIX-style (start with '/', no embedded '/' you care about).
    void setName(std::string name)
    {
        m_name = std::move(name);
    }

    [[nodiscard]] std::string name() const
    {
        return m_name;
    }


    // Notes name must be set via setName() first. mode/value only used if create flag is set.
    [[nodiscard]] JobSemRet open(JobSemFlags flags = JobSemFlags::None,
                                 mode_t mode = 0666,
                                 unsigned value = 0)
    {
        if (m_sem)
            return JobSemRet::Invalid;

        if (m_name.empty() || m_name[0] != '/') {
            JOB_LOG_ERROR("[JobSem] open() requires POSIX name starting with '/' (got '{}')", m_name);
            return JobSemRet::Invalid;
        }

        int oflag = 0;
        if (hasFlag(flags, JobSemFlags::Create))
            oflag |= O_CREAT;

        if (hasFlag(flags, JobSemFlags::Exclusive))
            oflag |= O_EXCL;

        errno = 0;
        sem_t *s = ::sem_open(m_name.c_str(), oflag, mode, value);
        if (s == SEM_FAILED)
            return fromErrno(errno);

        m_sem           = s;
        m_ready         = true;
        m_linked        = true;
        m_initedInPlace = false;
        m_autoUnlink    = hasFlag(flags, JobSemFlags::UnlinkOnDestroy);

        return JobSemRet::OK;
    }

    // This is for open(semi_open) ONLY for init use destroy.
    [[nodiscard]] JobSemRet close()
    {
        if (!m_sem)
            return JobSemRet::NotReady;

        if (!m_linked || m_initedInPlace)
            return JobSemRet::Invalid;

        if (::sem_close(m_sem) != 0)
            return fromErrno(errno);

        m_sem    = nullptr;
        m_ready  = false;
        m_linked = false;
        return JobSemRet::OK;
    }

    [[nodiscard]] JobSemRet unlink()
    {
        if (!m_linked || m_name.empty())
            return JobSemRet::Invalid;

        if (::sem_unlink(m_name.c_str()) != 0)
            return fromErrno(errno);

        return JobSemRet::OK;
    }

    //////////////////////////////////
    // ACTIONS (wait / timed wait / post)
    //////////////////////////////////

    // restart on EINTR
    [[nodiscard]] JobSemRet wait()
    {
        if (!sanity())
            return JobSemRet::NotReady;

        for (;;) {
            if (::sem_wait(m_sem) == 0)
                return JobSemRet::OK;

            if (errno == EINTR)
                continue;

            return fromErrno(errno);
        }
    }

    // Wait "value" times, sharing the same total timeout. try to acquire the semaphore 'value' times before timeout expires.
    [[nodiscard]] JobSemRet wait(int value, const PointDuration &timeout)
    {
        if (value <= 0)
            return JobSemRet::Invalid;

        // I didnt do the greatest here but split each acquire gets timeout / value
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

    // CLOCK_REALTIME + sem_timedwait()
    [[nodiscard]] JobSemRet wait(const PointDuration &timeout)
    {
        if (!sanity())
            return JobSemRet::NotReady;

        if (timeout.count() < 0)
            return JobSemRet::Invalid;

        ::timespec ts{};
        if (!makeAbsRealtime(timeout, ts))
            return JobSemRet::Invalid;

        for (;;) {
            if (::sem_timedwait(m_sem, &ts) == 0)
                return JobSemRet::OK;

            if (errno == EINTR)
                continue;

            if (errno == ETIMEDOUT)
                return JobSemRet::Timeout;

            return fromErrno(errno);
        }
    }

    // steady_clock
    [[nodiscard]] JobSemRet wait(const ClockDuration &till)
    {
        auto ms = std::chrono::duration_cast<PointDuration>(till);
        return wait(ms);
    }

    [[nodiscard]] JobSemRet post()
    {
        if (!sanity())
            return JobSemRet::NotReady;

        if (::sem_post(m_sem) == 0)
            return JobSemRet::OK;

        return fromErrno(errno);
    }

    //////////////////////////////////
    // META Information
    //////////////////////////////////

    [[nodiscard]] JobSemRet value(int &out) const
    {
        if (!sanity())
            return JobSemRet::NotReady;

        if (::sem_getvalue(m_sem, &out) == 0)
            return JobSemRet::OK;

        return fromErrno(errno);
    }

    [[nodiscard]] bool ready() const noexcept
    {
        return m_ready && m_sem != nullptr;
    }

private:
    [[nodiscard]] bool sanity() const noexcept
    {
        return m_ready && m_sem != nullptr;
    }

    [[nodiscard]] static JobSemRet fromErrno(int e)
    {
        switch (e) {
        case 0:
            return JobSemRet::OK;
        case EINVAL:
            return JobSemRet::Invalid;
        case ETIMEDOUT:
            return JobSemRet::Timeout;
        case EINTR:
            return JobSemRet::Interrupted;
        case EAGAIN:
            return JobSemRet::WouldBlock;
        case ENOENT:
            return JobSemRet::NotFound;
        case EEXIST:
            return JobSemRet::Exists;
        case ENOMEM:
            return JobSemRet::NoMemory;
        case EACCES:
            return JobSemRet::Permission;
        default:
            return JobSemRet::Unknown;
        }
    }

    [[nodiscard]] static bool makeAbsRealtime(const PointDuration &rel, ::timespec &out)
    {
        ::timespec now{};
        if (::clock_gettime(CLOCK_REALTIME, &now) != 0)
            return false;

        auto now_ns = std::chrono::seconds(now.tv_sec)
                      + std::chrono::nanoseconds(now.tv_nsec);

        auto rel_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(rel);
        auto abs_ns = now_ns + rel_ns;

        auto sec = std::chrono::duration_cast<std::chrono::seconds>(abs_ns);
        auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(abs_ns - sec);

        out.tv_sec = static_cast<time_t>(sec.count());
        out.tv_nsec = static_cast<long>(nsec.count());
        return true;
    }

    bool        m_linked{false};        // named semaphore opened via sem_open()
    bool        m_ready{false};         // we are open and ready to rock
    bool        m_initedInPlace{false}; // sem_init() vs sem_open()
    bool        m_autoUnlink{false};    // unlink on destroy() for named sems
    sem_t       *m_sem{nullptr};        // The POSIX semaphore
    sem_t       m_storage{};            // for in-place (unnamed) usage
    std::string m_name;                 // for named semaphores
};

} // namespace job::core
