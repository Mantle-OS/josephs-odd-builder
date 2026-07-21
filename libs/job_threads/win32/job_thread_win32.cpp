#include "job_thread.h"

#include <windows.h>
#include <process.h>
#include <cerrno>
#include <cstring>

#include "job_thread_args.h"

namespace job::threads {

static HANDLE &nativeHandle(unsigned char *storage) noexcept
{
    return *reinterpret_cast<HANDLE*>(storage);
}

// Windows has no SCHED_FIFO/SCHED_RR distinction for user threads -- only a
// priority level within a priority class. SchedulingPolicy is preserved for
// API/behavioral parity with the posix backend (and to keep JobThreadOptions
// meaningful there), but on Windows it only gates whether "realtime" mode is
// engaged; FIFO vs RoundRobin doesn't change anything here.
static int toNativePriority(std::uint8_t priority) noexcept
{
    if (priority >= 99)
        return THREAD_PRIORITY_TIME_CRITICAL;
    if (priority >= 80)
        return THREAD_PRIORITY_HIGHEST;
    if (priority >= 60)
        return THREAD_PRIORITY_ABOVE_NORMAL;
    if (priority >= 40)
        return THREAD_PRIORITY_NORMAL;
    if (priority >= 20)
        return THREAD_PRIORITY_BELOW_NORMAL;
    return THREAD_PRIORITY_IDLE;
}

using JobThreadArgs = ThreadArgs<JobThread, JobThread::StartResult>;

JobThread::JobThread(const JobThreadOptions &options) noexcept :
    m_options{options}
{
}

JobThread::~JobThread() noexcept
{
    requestStop();
    (void)join();
}

void JobThread::setOptions(const JobThreadOptions &options) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_options = options;
}

void JobThread::setRunFunction(RunFunction fn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_runFunc = std::move(fn);
}

JobThread::StartResult JobThread::start()
{
    if (m_running.load(std::memory_order_acquire) || m_starting.test_and_set(std::memory_order_acq_rel))
        return StartResult::AlreadyRunning;

    auto promise = std::make_shared<std::promise<StartResult>>();
    auto future  = promise->get_future();
    auto *args = new (std::nothrow)JobThreadArgs{
        this,
        promise,
        m_stopSource.get_token()
    };

    if (!args){
        m_starting.clear(std::memory_order_release);
        return StartResult::ThreadError;
    }

    uintptr_t create_result = _beginthreadex(nullptr, 0, &JobThread::threadEntry, args, 0, nullptr);

    if (create_result == 0) {
        delete args;
        m_joinable = false;
        promise->set_value(StartResult::ThreadError);
        m_starting.clear(std::memory_order_release);
    } else {
        nativeHandle(m_handleStorage) = reinterpret_cast<HANDLE>(create_result);
        m_joinable = true;
    }

    StartResult result = future.get();

    if (result != StartResult::Started && m_joinable) {
        HANDLE h = nativeHandle(m_handleStorage);
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
        m_joinable = false;
    }

    return result;
}

unsigned __stdcall JobThread::threadEntry(void *arg)
{
    std::unique_ptr<JobThreadArgs> args(static_cast<JobThreadArgs*>(arg));
    auto self = args->self;
    auto promise = args->promise;
    auto token = args->token;

    StartResult ret = StartResult::Started;
    {
        if (self->m_options.realtime) {
            std::lock_guard<std::mutex> lock(self->m_mutex);
            int ok_sched = self->applyScheduling();
            int ok_aff   = self->applyAffinity();
            if (ok_sched != 0)
                ret = StartResult::SchedulingFailed;
            else if (ok_aff != 0)
                ret = StartResult::AffinityFailed;
        }
    }

    promise->set_value(ret);

    if (ret == StartResult::Started) {
        self->m_running.store(true);
        self->m_starting.clear(std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(self->m_mutex);
            if (self->m_options.name[0] != '\0') {
                self->m_options.name[self->m_options.name.size() - 1] = '\0';

                // wchar_t wname[self->m_options.name.size()];
                const std::size_t nameSize = self->m_options.name.size();
                std::vector<wchar_t> wname(nameSize);

                std::size_t i = 0;
                for (; i < nameSize - 1 && self->m_options.name[i] != '\0'; ++i)
                    wname[i] = static_cast<wchar_t>(static_cast<unsigned char>(self->m_options.name[i]));
                wname[i] = L'\0';

                // Requires Windows 10 1607+ / Server 2016+
                SetThreadDescription(GetCurrentThread(), wname);
            }
        }
        RunFunction func_to_run;
        {
            std::lock_guard<std::mutex> lock(self->m_mutex);
            func_to_run = self->m_runFunc;
        }
        if (func_to_run)
            func_to_run(token);
        else
            self->run(token);

        self->m_running.store(false);
    }
    return 0;
}

void JobThread::requestStop() noexcept
{
    m_stopSource.request_stop();
}

bool JobThread::join() noexcept
{
    if (m_joinable) {
        HANDLE h = nativeHandle(m_handleStorage);
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
        m_joinable = false;
        return true;
    }
    return false;
}

bool JobThread::isRunning() const noexcept
{
    return m_running.load();
}

void JobThread::run(std::stop_token token) noexcept
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

// NOTE: m_mutex MUST be locked
int JobThread::applyScheduling() noexcept
{
    if (m_options.realtime) {
        if (m_options.lockMemory) {
            JOB_LOG_WARN("[JobThread] lockMemory requested but has no Windows equivalent to mlockall(); ignored");
        }

        if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
            JOB_LOG_ERROR("SetPriorityClass failed with Win32 Error: {}", GetLastError());
            return static_cast<int>(GetLastError());
        }

        if (!SetThreadPriority(GetCurrentThread(), toNativePriority(m_options.priority))) {
            JOB_LOG_ERROR("SetThreadPriority failed with Win32 Error: {}", GetLastError());
            return static_cast<int>(GetLastError());
        }
    }

    return 0;
}

// NOTE: m_mutex MUST be locked
int JobThread::applyAffinity() noexcept
{
    if (m_options.pinToCore) {
        if (m_options.coreId >= 64) {
            JOB_LOG_WARN("[JobThread] coreId {} exceeds single-mask affinity range (0-63) on Windows; "
                         "machines with >64 logical processors need the processor-group APIs (not implemented)",
                         m_options.coreId);
            return ERROR_INVALID_PARAMETER;
        }

        DWORD_PTR mask = static_cast<DWORD_PTR>(1) << m_options.coreId;
        if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0) {
            JOB_LOG_ERROR("SetThreadAffinityMask failed with Win32 Error: {}", GetLastError());
            return static_cast<int>(GetLastError());
        }
    }

    return 0;
}

} // namespace job::threads