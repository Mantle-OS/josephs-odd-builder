#pragma once

#include <functional>
#include <memory>

#include <libudev.h>

#include <job_io_async_thread.h>
#include <job_logger.h>

namespace job::input {

class JobUdevMonitor
{
public:
    using Ptr      = std::shared_ptr<JobUdevMonitor>;
    using WPtr     = std::weak_ptr<JobUdevMonitor>;
    using UPtr     = std::unique_ptr<JobUdevMonitor>;

    using Callback = std::function<void()>;
    using Loop     = std::weak_ptr<threads::JobIoAsyncThread>;

    JobUdevMonitor();
    ~JobUdevMonitor();

    JobUdevMonitor(const JobUdevMonitor &) = delete;
    JobUdevMonitor &operator=(const JobUdevMonitor &) = delete;
    JobUdevMonitor(JobUdevMonitor &&) = delete;
    JobUdevMonitor &operator=(JobUdevMonitor &&) = delete;

    [[nodiscard]] static Ptr  createShared();
    [[nodiscard]] static UPtr createUniq();

    [[nodiscard]] bool start(threads::JobIoAsyncThread::Ptr loop, const Callback &onDeviceChange);
    void stop();

private:
    void onEvents(threads::IOEvent events);

    Loop                m_loop;
    Callback            m_callback;
    struct udev         *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
    int                 m_udevFd = -1;
};

} // namespace job::input