#pragma once

#include <cstdint>
#include <functional>
#include <memory>


#include <job_io_async_thread.h>

#include "jobusb_export.h"

struct udev;
struct udev_monitor;

namespace job::usb {

enum class JobUsbMonitorAction : uint8_t
{
    Added = 0,
    Removed,
    Changed,
    Bound,
    Unbound,
    Unknown
};

class JOBUSB_EXPORT JobUsbMonitor
{
public:
    using Ptr  = std::shared_ptr<JobUsbMonitor>;
    using WPtr = std::weak_ptr<JobUsbMonitor>;
    using UPtr = std::unique_ptr<JobUsbMonitor>;
    using Loop = threads::JobIoAsyncThread::WPtr;

    using Callback = std::function<void(JobUsbMonitorAction action, std::string_view sysfsPath)>;

    JobUsbMonitor();
    explicit JobUsbMonitor(const threads::JobIoAsyncThread::Ptr &loop);

    ~JobUsbMonitor();

    JobUsbMonitor(const JobUsbMonitor &) = delete;
    JobUsbMonitor &operator=(const JobUsbMonitor &) = delete;
    JobUsbMonitor(JobUsbMonitor &&) = delete;
    JobUsbMonitor &operator=(JobUsbMonitor &&) = delete;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static Ptr createShared(const threads::JobIoAsyncThread::Ptr &loop);

    [[nodiscard]] static UPtr createUniq();
    [[nodiscard]] static UPtr createUniq(const threads::JobIoAsyncThread::Ptr &loop);

    [[nodiscard]] bool start(Callback callback);
    [[nodiscard]] bool start(const threads::JobIoAsyncThread::Ptr &loop, Callback callback);

    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

private:
    void onEvents(threads::IOEvent events);
    void drain();

    Loop            m_loop;
    udev            *m_udev = nullptr;
    udev_monitor    *m_monitor = nullptr;
    Callback        m_callback;
    int             m_fd = -1;
    bool            m_running = false;
};

} // namespace job::usb