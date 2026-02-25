#include "udev_monitor_thread.h"
#include <poll.h>
#include <unistd.h>
#include <cstring>

namespace job::uart {

UdevMonitorThread::UdevMonitorThread()
{
    m_udev = udev_new();
    if (!m_udev)
        JOB_LOG_ERROR("[UdevMonitor] Failed to create udev context.");
}

UdevMonitorThread::~UdevMonitorThread()
{
    stop();

    if (m_monitor)
        udev_monitor_unref(m_monitor);
    if (m_udev)
        udev_unref(m_udev);
}

bool UdevMonitorThread::start(std::shared_ptr<threads::JobIoAsyncThread> loop, const Callback &onDeviceChange)
{
    if (!m_udev) return false;
    if (!loop || !loop->isRunning()) {
        JOB_LOG_ERROR("[UdevMonitor] Event loop is not valid or not running.");
        return false;
    }

    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_monitor) {
        JOB_LOG_ERROR("[UdevMonitor] Failed to create udev monitor.");
        return false;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "tty", nullptr);
    udev_monitor_enable_receiving(m_monitor);

    m_udevFd = udev_monitor_get_fd(m_monitor);
    if (m_udevFd < 0) {
        JOB_LOG_ERROR("[UdevMonitor] Failed to get udev monitor FD.");
        return false;
    }

    m_loop = loop; // Store weak_ptr
    m_callback = onDeviceChange;

    if (!loop->registerFD(m_udevFd, EPOLLIN, [this](uint32_t e) { onEvents(e); })) {
        JOB_LOG_ERROR("[UdevMonitor] Failed to register FD with event loop!");
        m_udevFd = -1;
        return false;
    }

    return true;
}

void UdevMonitorThread::stop()
{
    if (auto loop = m_loop.lock()) {
        if (m_udevFd != -1) {
            loop->unregisterFD(m_udevFd);
            m_udevFd = -1;
        }
    }
    m_callback = nullptr;
}

void UdevMonitorThread::onEvents(uint32_t events)
{
    if (events & POLLIN) {
        if (struct udev_device *dev = udev_monitor_receive_device(m_monitor)) {
            const char *action = udev_device_get_action(dev);

            if (action && (strcmp(action, "add") == 0 || strcmp(action, "remove") == 0))
                if (m_callback)
                    m_callback();

            udev_device_unref(dev);
        }
    }

    if (events & (POLLERR | POLLHUP)) {
        JOB_LOG_ERROR("[UdevMonitor] Error on udev FD, attempting to stop and restart listener.");
        stop();
    }
}

} // namespace job::uart

