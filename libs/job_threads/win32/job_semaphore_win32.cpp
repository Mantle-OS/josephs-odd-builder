#include "job_semaphore.h"

#include <windows.h>

#include <algorithm>
#include <climits>
#include <limits>
#include <string>

namespace job::threads {

namespace {

[[nodiscard]] std::wstring widenAscii(const std::string &text)
{
    std::wstring result(text.size(), L'\0');

    for (std::size_t index = 0; index < text.size(); ++index)
        result[index] = static_cast<wchar_t>( static_cast<unsigned char>(text[index]));

    return result;
}

[[nodiscard]] bool validSemaphoreValue(unsigned value) noexcept
{
    return value <= static_cast<unsigned>(LONG_MAX);
}

[[nodiscard]] DWORD waitTimeoutMilliseconds(const JobSem::PointDuration &timeout) noexcept
{
    constexpr auto maximumTimeout = static_cast<long long>(INFINITE - 1);

    auto const milliseconds = std::min<long long>(timeout.count(), maximumTimeout);

    return static_cast<DWORD>(milliseconds);
}

} // namespace

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

    // Windows has no POSIX-style sem_unlink() operation.
    if (!::CloseHandle(static_cast<HANDLE>(m_sem)))
        JOB_LOG_WARN("[JobSem] CloseHandle in destructor failed: {}", ::GetLastError());
}

JobSemRet JobSem::init(unsigned value, bool pshared)
{
    if (m_sem) {
        JOB_LOG_WARN("[JobSem] init() called on already-initialized semaphore");
        return JobSemRet::Invalid;
    }

    if (!validSemaphoreValue(value))
        return JobSemRet::Invalid;

    if (pshared)
        JOB_LOG_WARN("[JobSem] pshared=true has no unnamed Windows equivalent; ");

    HANDLE const handle = ::CreateSemaphoreW(nullptr, static_cast<LONG>(value), LONG_MAX, nullptr);

    if (!handle)
        return fromPlatformError(static_cast<long>(::GetLastError()));

    m_sem           = handle;
    m_ready         = true;
    m_linked        = false;
    m_initedInPlace = true;
    m_autoUnlink    = false;

    m_shadowCount.store(static_cast<int>(value), std::memory_order_relaxed);

    return JobSemRet::OK;
}

JobSemRet JobSem::destroy()
{
    if (!m_sem)
        return JobSemRet::NotReady;


    // Do not clear wrapper ownership when CloseHandle() fails.
    // A failed close does not prove that the native HANDLE has been
    // released. Keeping m_sem prevents a later init() or open() from
    // silently replacing a handle that may still be owned.
    if (!::CloseHandle(static_cast<HANDLE>(m_sem)))
        return fromPlatformError(static_cast<long>(::GetLastError()));

    m_sem           = nullptr;
    m_ready         = false;
    m_initedInPlace = false;
    m_linked        = false;
    m_autoUnlink    = false;

    m_shadowCount.store(0, std::memory_order_relaxed);

    return JobSemRet::OK;
}

JobSemRet JobSem::open(JobSemFlags flags, unsigned mode, unsigned value)
{
    (void)mode;

    if (m_sem)
        return JobSemRet::Invalid;

    if (m_name.empty() || m_name[0] != '/') {
        JOB_LOG_ERROR(
            "[JobSem] open() requires POSIX name starting with '/' "
            "(got '{}')",
            m_name
            );

        return JobSemRet::Invalid;
    }

    if (!validSemaphoreValue(value))
        return JobSemRet::Invalid;

    std::wstring const nativeName = L"Local\\" + widenAscii(m_name);

    HANDLE handle = nullptr;
    bool createdNew = false;

    if (hasFlag(flags, JobSemFlags::Create)) {
        // GetLastError() must be inspected immediately after a successful
        // CreateSemaphoreW() because ERROR_ALREADY_EXISTS distinguishes a
        // newly created object from an existing named object.

        ::SetLastError(ERROR_SUCCESS);

        handle = ::CreateSemaphoreW(nullptr, static_cast<LONG>(value), LONG_MAX, nativeName.c_str());

        if (!handle)
            return fromPlatformError(static_cast<long>(::GetLastError()));

        DWORD const createStatus = ::GetLastError();
        bool const alreadyExists = createStatus == ERROR_ALREADY_EXISTS;

        createdNew = !alreadyExists;

        if (alreadyExists &&
            hasFlag(flags, JobSemFlags::Exclusive)) {
            if (!::CloseHandle(handle)) {

                JOB_LOG_WARN(
                    "[JobSem] CloseHandle after exclusive-create "
                    "collision failed: {}",
                    ::GetLastError()
                    );

            }

            return JobSemRet::Exists;
        }
    } else {
        constexpr DWORD desiredAccess = SYNCHRONIZE | SEMAPHORE_MODIFY_STATE;
        handle = ::OpenSemaphoreW( desiredAccess, FALSE, nativeName.c_str());

        if (!handle)
            return fromPlatformError(static_cast<long>(::GetLastError()));
    }

    m_sem           = handle;
    m_ready         = true;
    m_linked        = true;
    m_initedInPlace = false;
    m_autoUnlink    =
        hasFlag(flags, JobSemFlags::UnlinkOnDestroy);

    m_shadowCount.store(static_cast<int>(value), std::memory_order_relaxed);

    if (!createdNew) {
        JOB_LOG_WARN(
            "[JobSem] open() attached to a pre-existing named semaphore "
            "'{}'. Windows cannot query its current count; value() is a "
            "wrapper-local approximation for this handle.",
            m_name
            );
    }

    return JobSemRet::OK;
}

JobSemRet JobSem::close()
{
    if (!m_sem)
        return JobSemRet::NotReady;

    if (!m_linked || m_initedInPlace)
        return JobSemRet::Invalid;

    if (!::CloseHandle(static_cast<HANDLE>(m_sem)))
        return fromPlatformError(static_cast<long>(::GetLastError()));

    m_sem           = nullptr;
    m_ready         = false;
    m_linked        = false;
    m_initedInPlace = false;
    m_autoUnlink    = false;

    m_shadowCount.store(0, std::memory_order_relaxed);

    return JobSemRet::OK;
}

JobSemRet JobSem::unlink()
{
    if (!m_linked || m_name.empty())
        return JobSemRet::Invalid;

    // There is no operation equivalent to POSIX sem_unlink()
    return JobSemRet::OK;
}

JobSemRet JobSem::wait()
{
    if (!sanity())
        return JobSemRet::NotReady;

    DWORD const result = ::WaitForSingleObject(static_cast<HANDLE>(m_sem), INFINITE);

    if (result == WAIT_OBJECT_0) {
        m_shadowCount.fetch_sub(1, std::memory_order_relaxed);
        return JobSemRet::OK;
    }

    if (result == WAIT_FAILED)
        return fromPlatformError(static_cast<long>(::GetLastError()));


    // WAIT_ABANDONED applies to mutex objects, not semaphores. Any other result indicates an unexpected provider condition.
    return JobSemRet::Unknown;
}

JobSemRet JobSem::wait(const PointDuration &timeout)
{
    if (!sanity())
        return JobSemRet::NotReady;

    if (timeout.count() < 0)
        return JobSemRet::Invalid;

    DWORD const result = ::WaitForSingleObject(static_cast<HANDLE>(m_sem), waitTimeoutMilliseconds(timeout));

    if (result == WAIT_OBJECT_0) {
        m_shadowCount.fetch_sub(1, std::memory_order_relaxed);
        return JobSemRet::OK;
    }

    if (result == WAIT_TIMEOUT)
        return JobSemRet::Timeout;

    if (result == WAIT_FAILED)
        return fromPlatformError(static_cast<long>(::GetLastError()));

    return JobSemRet::Unknown;
}

JobSemRet JobSem::post()
{
    if (!sanity())
        return JobSemRet::NotReady;

    LONG previousCount = 0;

    if (!::ReleaseSemaphore(static_cast<HANDLE>(m_sem), 1, &previousCount))
        return fromPlatformError(static_cast<long>(::GetLastError()));

    m_shadowCount.store(static_cast<int>(previousCount + 1), std::memory_order_relaxed);
    return JobSemRet::OK;
}

JobSemRet JobSem::value(int &out) const
{
    if (!sanity())
        return JobSemRet::NotReady;

    out = m_shadowCount.load(std::memory_order_relaxed);
    return JobSemRet::OK;
}

JobSemRet JobSem::fromPlatformError(long error)
{
    switch (static_cast<DWORD>(error)) {
    case ERROR_SUCCESS:
        return JobSemRet::OK;
    case ERROR_ALREADY_EXISTS:
        return JobSemRet::Exists;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return JobSemRet::NotFound;
    case ERROR_INVALID_HANDLE:
    case ERROR_INVALID_PARAMETER:
    case ERROR_TOO_MANY_POSTS:
        return JobSemRet::Invalid;
    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
        return JobSemRet::Permission;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
    case ERROR_NO_SYSTEM_RESOURCES:
        return JobSemRet::NoMemory;
    case WAIT_TIMEOUT:
        return JobSemRet::Timeout;

    default:
        return JobSemRet::Unknown;
    }
}

} // namespace job::threads