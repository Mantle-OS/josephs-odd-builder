#pragma once

#include <atomic>
#include <functional>
#include <libudev.h>
#include "job_thread.h"

namespace job::uart {

class UdevMonitorThread {
public:
    using Callback = std::function<void()>;

    UdevMonitorThread();
    ~UdevMonitorThread();

    void start(const Callback &onDeviceChange);
    void stop();

private:
    void monitorLoop(std::stop_token token);

    std::shared_ptr<threads::JobThread> m_thread;
    std::atomic<bool> m_running{false};
    Callback m_callback;

    struct udev *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
};

} // namespace job::uart
