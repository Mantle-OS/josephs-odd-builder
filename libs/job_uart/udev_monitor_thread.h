#pragma once

#include <functional>
#include <memory>
#include <libudev.h>

#include <job_io_async_thread.h>
#include <job_logger.h>

namespace job::uart {

class UdevMonitorThread {
public:
    using Callback = std::function<void()>;

    UdevMonitorThread();
    ~UdevMonitorThread();

    bool start(std::shared_ptr<threads::JobIoAsyncThread> loop, const Callback &onDeviceChange);
    void stop();

private:
    void onEvents(uint32_t events);
    Callback m_callback;
    std::weak_ptr<threads::JobIoAsyncThread> m_loop; // Store a weak_ptr

    struct udev *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
    int m_udevFd = -1;
};

} // namespace job::uart
// CHECKPOINT: v2.0
