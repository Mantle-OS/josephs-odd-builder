#include "job_thread.h"

#include <windows.h>
#include <process.h>
#include <vector>

#include "job_thread_args.h"

namespace job::threads {

static HANDLE &nativeHandle(unsigned char *storage) noexcept
{
    return *reinterpret_cast<HANDLE*>(storage);
}

static int toNativePriority(std::uint8_t priority) noexcept
{
    if (priority >= 99) return THREAD_PRIORITY_TIME_CRITICAL;
    if (priority >= 80) return THREAD_PRIORITY_HIGHEST;
    if (priority >= 60) return THREAD_PRIORITY_ABOVE_NORMAL;
    if (priority >= 40) return THREAD_PRIORITY_NORMAL;
    if (priority >= 20) return THREAD_PRIORITY_BELOW_NORMAL;
    return THREAD_PRIORITY_IDLE;
}

using JobThreadArgs = ThreadArgs<JobThread, JobThread::StartResult>;

JobThread::StartResult JobThread::start()
{
    if (m_running.load(std::memory_order_acquire) || m_starting.test_and_set(std::memory_order_acq_rel))
        return StartResult::AlreadyRunning;

    auto promise = std::make_shared<std::promise<StartResult>>();
    auto future  = promise->get_future();
    auto *args = new (std::nothrow) JobThreadArgs {
        this,
        promise,
        m_stopSource.get_token()
    };

    if (!args) {
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
        std::lock_guard<std::mutex> lock(self->m_mutex);
        if (self->m_options.realtime || self->m_options.priority != 0)
            if (self->applyScheduling() != 0)
                ret = StartResult::SchedulingFailed;

        if (ret == StartResult::Started && self->m_options.pinToCore)
            if (self->applyAffinity() != 0)
                ret = StartResult::AffinityFailed;
    }

    promise->set_value(ret);

    if (ret != StartResult::Started) {
        self->m_starting.clear(std::memory_order_release);
        return 0;
    }

    self->m_running.store(true, std::memory_order_release);
    self->m_starting.clear(std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        if (self->m_options.name[0] != '\0') {
            const std::size_t nameSize = self->m_options.name.size();
            std::vector<wchar_t> wname(nameSize);

            std::size_t i = 0;
            for (; i < nameSize - 1 && self->m_options.name[i] != '\0'; ++i)
                wname[i] = static_cast<wchar_t>(static_cast<unsigned char>(self->m_options.name[i]));

            wname[i] = L'\0';

            SetThreadDescription(GetCurrentThread(), wname.data());
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

    self->m_running.store(false, std::memory_order_release);
    return 0;
}

int JobThread::applyScheduling() noexcept
{
    if (!m_options.valid())
        return ERROR_INVALID_PARAMETER;

    if (m_options.realtime) {
        // Logging or no-op here is expected.
        if (m_options.lockMemory)
            JOB_LOG_DEBUG("VirtualLock could lock a specific range, but Windows has no global mlockall()");

        if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
            return static_cast<int>(GetLastError());
    }

    if (m_options.priority != 0)
        if (!SetThreadPriority(GetCurrentThread(), toNativePriority(m_options.priority)))
            return static_cast<int>(GetLastError());

    return 0;
}

int JobThread::applyAffinity() noexcept
{
    if (m_options.pinToCore) {
        if (m_options.coreId == JobThreadOptions::kCoreUnbound || m_options.coreId >= 64)
            return ERROR_INVALID_PARAMETER;

        DWORD_PTR mask = static_cast<DWORD_PTR>(1) << m_options.coreId;
        if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0)
            return static_cast<int>(GetLastError());
    }

    return 0;
}

} // namespace job::threads