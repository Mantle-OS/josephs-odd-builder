#include "job_semaphore.h"

#include <windows.h>
#include <string>

namespace job::threads {

static std::wstring widenAscii(const std::string &s)
{
    std::wstring w(s.size(), L'\0');
    for (std::size_t i = 0; i < s.size(); ++i)
        w[i] = static_cast<wchar_t>(static_cast<unsigned char>(s[i]));
    return w;
}

JobSem::JobSem(unsigned value, bool autoInit)
{
    if (autoInit)
        if (init(value) != JobSemRet::OK)
            JOB_LOG_WARN("[JobSem] Failed to auto init the semaphore");
}

JobSem::~JobSem()
{
    if (!m_sem)
        return;

    // Both named and in-place modes are just a HANDLE on Windows --
    // CloseHandle covers both, unlike POSIX's sem_destroy/sem_close split.
    if (!CloseHandle(static_cast<HANDLE>(m_sem)))
        JOB_LOG_WARN("[JobSem] CloseHandle in destructor failed: {}", GetLastError());
}

JobSemRet JobSem::init(unsigned value, bool pshared)
{
    if (m_sem) {
        JOB_LOG_WARN("[JobSem] init() called on already-initialized semaphore");
        return JobSemRet::Invalid;
    }

    if (pshared) {
        JOB_LOG_WARN("[JobSem] pshared=true has no Windows equivalent (a Windows "
                     "semaphore is always an opaque per-process HANDLE, never "
                     "memory-mappable bytes); falling back to a local (non-shared) "
                     "semaphore. Use the named open()/close() path for real "
                     "cross-process sharing, as job_shared_memory already does.");
    }

    HANDLE h = CreateSemaphoreW(nullptr, static_cast<LONG>(value), LONG_MAX, nullptr);
    if (!h)
        return fromPlatformError(static_cast<long>(GetLastError()));

    m_sem           = h;
    m_ready         = true;
    m_linked        = false;
    m_initedInPlace = true;
    m_shadowCount.store(static_cast<int>(value), std::memory_order_relaxed);

    return JobSemRet::OK;
}

JobSemRet JobSem::destroy()
{
    if (!m_sem)
        return JobSemRet::NotReady;

    JobSemRet result = JobSemRet::OK;
    if (!CloseHandle(static_cast<HANDLE>(m_sem)))
        result = fromPlatformError(static_cast<long>(GetLastError()));

    // unlink() is a documented no-op on Windows (see job_semaphore.h) so
    // m_autoUnlink needs no action here.

    m_sem           = nullptr;
    m_ready         = false;
    m_initedInPlace = false;
    m_linked        = false;

    return result;
}


JobSemRet JobSem::open(JobSemFlags flags, unsigned mode, unsigned value)
{
    (void)mode; // no permission-bits concept for named kernel objects here

    if (m_sem)
        return JobSemRet::Invalid;

    if (m_name.empty() || m_name[0] != '/') {
        JOB_LOG_ERROR("[JobSem] open() requires POSIX name starting with '/' (got '{}')", m_name);
        return JobSemRet::Invalid;
    }

    std::wstring wname = L"Local\\" + widenAscii(m_name);

    HANDLE h = nullptr;
    bool weCreatedTheObject = false;

    if (hasFlag(flags, JobSemFlags::Create)) {
        h = CreateSemaphoreW(nullptr, static_cast<LONG>(value), LONG_MAX, wname.c_str());
        if (h) {
            const bool alreadyExisted = (GetLastError() == ERROR_ALREADY_EXISTS);
            weCreatedTheObject = !alreadyExisted;

            if (alreadyExisted && hasFlag(flags, JobSemFlags::Exclusive)) {
                CloseHandle(h);
                return JobSemRet::Exists;
            }
        }
    } else {
        h = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, wname.c_str());
    }

    if (!h)
        return fromPlatformError(static_cast<long>(GetLastError()));

    m_sem           = h;
    m_ready         = true;
    m_linked        = true;
    m_initedInPlace = false;
    m_autoUnlink    = hasFlag(flags, JobSemFlags::UnlinkOnDestroy);

    if (!weCreatedTheObject) {
        JOB_LOG_WARN("[JobSem] open() attached to a pre-existing named semaphore '{}' "
                     "rather than creating it. The OS ignored the requested initial "
                     "value, and this backend has no way to read the object's true "
                     "current count. value() will report an approximation seeded from "
                     "the requested value until enough post()/wait() calls occur.", m_name);
    }
    m_shadowCount.store(static_cast<int>(value), std::memory_order_relaxed);

    return JobSemRet::OK;
}

JobSemRet JobSem::close()
{
    if (!m_sem)
        return JobSemRet::NotReady;

    if (!m_linked || m_initedInPlace)
        return JobSemRet::Invalid;

    if (!CloseHandle(static_cast<HANDLE>(m_sem)))
        return fromPlatformError(static_cast<long>(GetLastError()));

    m_sem    = nullptr;
    m_ready  = false;
    m_linked = false;
    return JobSemRet::OK;
}

JobSemRet JobSem::unlink()
{
    if (!m_linked || m_name.empty())
        return JobSemRet::Invalid;

    // See job_semaphore.h's doc comment on unlink(): named kernel objects
    // vanish once the last handle closes, so there's nothing left to do.
    return JobSemRet::OK;
}

JobSemRet JobSem::wait()
{
    if (!sanity())
        return JobSemRet::NotReady;

    DWORD r = WaitForSingleObject(static_cast<HANDLE>(m_sem), INFINITE);
    if (r == WAIT_OBJECT_0) {
        m_shadowCount.fetch_sub(1, std::memory_order_relaxed);
        return JobSemRet::OK;
    }
    return fromPlatformError(static_cast<long>(GetLastError()));
}

JobSemRet JobSem::wait(const PointDuration &timeout)
{
    if (!sanity())
        return JobSemRet::NotReady;

    if (timeout.count() < 0)
        return JobSemRet::Invalid;

    DWORD ms = static_cast<DWORD>(std::min<long long>(timeout.count(),
                                                      static_cast<long long>(INFINITE - 1)));
    DWORD r = WaitForSingleObject(static_cast<HANDLE>(m_sem), ms);

    if (r == WAIT_OBJECT_0) {
        m_shadowCount.fetch_sub(1, std::memory_order_relaxed);
        return JobSemRet::OK;
    }
    if (r == WAIT_TIMEOUT)
        return JobSemRet::Timeout;

    return fromPlatformError(static_cast<long>(GetLastError()));
}

JobSemRet JobSem::post()
{
    if (!sanity())
        return JobSemRet::NotReady;

    if (ReleaseSemaphore(static_cast<HANDLE>(m_sem), 1, nullptr)) {
        m_shadowCount.fetch_add(1, std::memory_order_relaxed);
        return JobSemRet::OK;
    }

    return fromPlatformError(static_cast<long>(GetLastError()));
}

JobSemRet JobSem::value(int &out) const
{
    if (!sanity())
        return JobSemRet::NotReady;

    // See job_semaphore.h's doc comment on m_shadowCount.
    out = m_shadowCount.load(std::memory_order_relaxed);
    return JobSemRet::OK;
}

JobSemRet JobSem::fromPlatformError(long e)
{
    switch (static_cast<DWORD>(e)) {
    case ERROR_SUCCESS:           return JobSemRet::OK;
    case ERROR_ALREADY_EXISTS:    return JobSemRet::Exists;
    case ERROR_FILE_NOT_FOUND:    return JobSemRet::NotFound;
    case ERROR_INVALID_HANDLE:
    case ERROR_INVALID_PARAMETER: return JobSemRet::Invalid;
    case ERROR_ACCESS_DENIED:     return JobSemRet::Permission;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:       return JobSemRet::NoMemory;
    case WAIT_TIMEOUT:            return JobSemRet::Timeout;
    default:                      return JobSemRet::Unknown;
    }
}

} // namespace job::threads