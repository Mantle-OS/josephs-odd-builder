#include "job_semaphore.h"

#include <semaphore.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <ctime>

namespace job::threads {

static sem_t *provider(unsigned char *hS) noexcept
{
    return reinterpret_cast<sem_t*>(hS);
}

static bool makeAbsRealtime(const JobSem::PointDuration &rel, ::timespec &out)
{
    ::timespec now{};
    if (::clock_gettime(CLOCK_REALTIME, &now) != 0)
        return false;

    auto now_ns = std::chrono::seconds(now.tv_sec) + std::chrono::nanoseconds(now.tv_nsec);
    auto rel_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(rel);
    auto abs_ns = now_ns + rel_ns;

    auto sec  = std::chrono::duration_cast<std::chrono::seconds>(abs_ns);
    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(abs_ns - sec);

    out.tv_sec  = static_cast<time_t>(sec.count());
    out.tv_nsec = static_cast<long>(nsec.count());
    return true;
}

// Should be in shared header maybe
JobSem::JobSem(unsigned value, bool autoInit)
{
    if (autoInit)
        if (init(value) != JobSemRet::OK)
            JOB_LOG_WARN("[JobSem] Failed to auto init the semaphore");
}

// STAY
JobSem::~JobSem()
{
    if (!m_sem)
        return;

    if (!m_linked && m_initedInPlace) {
        if (::sem_destroy(provider(m_inPlaceStorage)) != 0)
            JOB_LOG_WARN("[JobSem] sem_destroy in destructor failed: {}", std::strerror(errno));

        return;
    }

    if (::sem_close(static_cast<sem_t *>(m_sem)) != 0)
        JOB_LOG_WARN("[JobSem] sem_close in destructor failed: {}", std::strerror(errno));

    if (m_autoUnlink && !m_name.empty())
        if (::sem_unlink(m_name.c_str()) != 0 && errno != ENOENT)
            JOB_LOG_WARN("[JobSem] sem_unlink in destructor failed: {}", std::strerror(errno));
}
// STAY
JobSemRet JobSem::init(unsigned value, bool pshared)
{
    if (m_sem) {
        JOB_LOG_WARN("[JobSem] init() called on already-initialized semaphore");
        return JobSemRet::Invalid;
    }

    int rc = ::sem_init(provider(m_inPlaceStorage), pshared ? 1 : 0, value);
    if (rc == 0) {
        m_sem           = provider(m_inPlaceStorage);
        m_ready         = true;
        m_linked        = false;
        m_initedInPlace = true;
        return JobSemRet::OK;
    }
    return fromPlatformError(errno);
}

// STAY
JobSemRet JobSem::destroy()
{
    if (!m_sem)
        return JobSemRet::NotReady;

    if (!m_linked && m_initedInPlace) {
        if (::sem_destroy(provider(m_inPlaceStorage)) != 0)
            return fromPlatformError(errno);

        m_sem           = nullptr;
        m_ready         = false;
        m_initedInPlace = false;
        m_linked        = false;
        m_autoUnlink    = false;

        return JobSemRet::OK;
    }

    if (::sem_close(static_cast<sem_t *>(m_sem)) != 0)
        return fromPlatformError(errno);

    JobSemRet unlinkResult = JobSemRet::OK;

    if (m_autoUnlink && !m_name.empty()) {
        if (::sem_unlink(m_name.c_str()) != 0 && errno != ENOENT)
            unlinkResult = fromPlatformError(errno);
    }

    m_sem           = nullptr;
    m_ready         = false;
    m_initedInPlace = false;
    m_linked        = false;
    m_autoUnlink    = false;

    return unlinkResult;
}

// STAY
JobSemRet JobSem::open(JobSemFlags flags, unsigned mode, unsigned value)
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
    sem_t *s = ::sem_open(m_name.c_str(), oflag, static_cast<mode_t>(mode), value);
    if (s == SEM_FAILED)
        return fromPlatformError(errno);

    m_sem           = s;
    m_ready         = true;
    m_linked        = true;
    m_initedInPlace = false;
    m_autoUnlink    = hasFlag(flags, JobSemFlags::UnlinkOnDestroy);

    return JobSemRet::OK;
}

// STAY
JobSemRet JobSem::close()
{
    if (!m_sem)
        return JobSemRet::NotReady;

    if (!m_linked || m_initedInPlace)
        return JobSemRet::Invalid;

    if (::sem_close(static_cast<sem_t*>(m_sem)) != 0)
        return fromPlatformError(errno);

    m_sem           = nullptr;
    m_ready         = false;
    m_linked        = false;
    m_initedInPlace = false;
    m_autoUnlink    = false;
    return JobSemRet::OK;
}

// STAY
JobSemRet JobSem::unlink()
{
    if (!m_linked || m_name.empty())
        return JobSemRet::Invalid;

    if (::sem_unlink(m_name.c_str()) != 0)
        return fromPlatformError(errno);

    return JobSemRet::OK;
}

// STAY
JobSemRet JobSem::wait()
{
    if (!sanity())
        return JobSemRet::NotReady;

    for (;;) {
        if (::sem_wait(static_cast<sem_t*>(m_sem)) == 0)
            return JobSemRet::OK;

        if (errno == EINTR)
            continue;

        return fromPlatformError(errno);
    }
}

// STAY
JobSemRet JobSem::wait(const PointDuration &timeout)
{
    if (!sanity())
        return JobSemRet::NotReady;

    if (timeout.count() < 0)
        return JobSemRet::Invalid;

    ::timespec ts{};
    if (!makeAbsRealtime(timeout, ts))
        return JobSemRet::Invalid;

    for (;;) {
        if (::sem_timedwait(static_cast<sem_t*>(m_sem), &ts) == 0)
            return JobSemRet::OK;

        if (errno == EINTR)
            continue;

        if (errno == ETIMEDOUT)
            return JobSemRet::Timeout;

        return fromPlatformError(errno);
    }
}

// STAY
JobSemRet JobSem::post()
{
    if (!sanity())
        return JobSemRet::NotReady;

    if (::sem_post(static_cast<sem_t*>(m_sem)) == 0)
        return JobSemRet::OK;

    return fromPlatformError(errno);
}

// STAY
JobSemRet JobSem::value(int &out) const
{
    if (!sanity())
        return JobSemRet::NotReady;

    if (::sem_getvalue(static_cast<sem_t*>(m_sem), &out) == 0)
        return JobSemRet::OK;

    return fromPlatformError(errno);
}

// STAY
JobSemRet JobSem::fromPlatformError(long e)
{
    switch (e) {
    case 0:          return JobSemRet::OK;
    case EINVAL:     return JobSemRet::Invalid;
    case ETIMEDOUT:  return JobSemRet::Timeout;
    case EINTR:      return JobSemRet::Interrupted;
    case EAGAIN:     return JobSemRet::WouldBlock;
    case ENOENT:     return JobSemRet::NotFound;
    case EEXIST:     return JobSemRet::Exists;
    case ENOMEM:     return JobSemRet::NoMemory;
    case EACCES:     return JobSemRet::Permission;
    default:         return JobSemRet::Unknown;
    }
}

} // namespace job::threads