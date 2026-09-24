#include "job_udev_monitor.h"

#include <cstring>

namespace job::input {

JobUdevMonitor::JobUdevMonitor()
{
    m_udev = udev_new();

    if (!m_udev)
        JOB_LOG_ERROR("[JobUdevMonitor] Failed to create udev context.");
}

JobUdevMonitor::~JobUdevMonitor()
{
    stop();

    if (m_monitor)
        udev_monitor_unref(m_monitor);

    if (m_udev)
        udev_unref(m_udev);
}

JobUdevMonitor::Ptr JobUdevMonitor::createShared()
{
    return std::make_shared<JobUdevMonitor>();
}

JobUdevMonitor::UPtr JobUdevMonitor::createUniq()
{
    return std::make_unique<JobUdevMonitor>();
}

bool JobUdevMonitor::start(threads::JobIoAsyncThread::Ptr loop, const Callback &onDeviceChange)
{
    if (!m_udev)
        return false;

    if (!loop || !loop->isRunning()) {
        JOB_LOG_ERROR("[JobUdevMonitor] Event loop is not valid or not running.");
        return false;
    }

    if (m_monitor)
        return true;

    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");

    if (!m_monitor) {
        JOB_LOG_ERROR("[JobUdevMonitor] Failed to create udev monitor.");
        return false;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "input", nullptr);
    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "drm", nullptr);
    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "usb", nullptr);
    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "tty", nullptr);

    if (udev_monitor_enable_receiving(m_monitor) < 0) {
        JOB_LOG_ERROR("[JobUdevMonitor] Failed to enable udev monitor.");

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    m_udevFd = udev_monitor_get_fd(m_monitor);

    if (m_udevFd < 0) {
        JOB_LOG_ERROR("[JobUdevMonitor] Failed to get udev monitor FD.");

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    m_loop = loop;
    m_callback = onDeviceChange;

    if (!loop->registerFD(m_udevFd, threads::IOEvent::Read, [this](threads::IOEvent events) { onEvents(events); })) {
        JOB_LOG_ERROR("[JobUdevMonitor] Failed to register FD with event loop.");

        m_callback = nullptr;
        m_loop.reset();
        m_udevFd = -1;

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    return true;
}

void JobUdevMonitor::stop()
{
    if (auto loop = m_loop.lock()) {
        if (m_udevFd != -1) {
            if (!loop->unregisterFD(m_udevFd))
                JOB_LOG_WARN("[JobUdevMonitor] Failed to unregister fd {}", m_udevFd);
        }
    }

    m_udevFd = -1;
    m_loop.reset();
    m_callback = nullptr;

    if (m_monitor) {
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
    }
}

void JobUdevMonitor::onEvents(threads::IOEvent events)
{
    if (threads::hasEvent(events, threads::IOEvent::Error) ||
        threads::hasEvent(events, threads::IOEvent::HangUp)) {
        JOB_LOG_ERROR("[JobUdevMonitor] Error or hangup on udev FD.");
        stop();
        return;
    }

    if (!threads::hasEvent(events, threads::IOEvent::Read))
        return;

    for (;;) {
        struct udev_device *dev = udev_monitor_receive_device(m_monitor);

        if (!dev)
            break;

        const char *action = udev_device_get_action(dev);
        const bool topologyChanged = action && (std::strcmp(action, "add") == 0 || std::strcmp(action, "remove") == 0 || std::strcmp(action, "change") == 0);

        if (topologyChanged && m_callback)
            m_callback();

        udev_device_unref(dev);
    }
}

} // namespace job::input