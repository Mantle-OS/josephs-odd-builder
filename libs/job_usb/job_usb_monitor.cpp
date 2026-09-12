#include "job_usb_monitor.h"

#include <cstring>
#include <utility>

#include <libudev.h>

#include <job_logger.h>

namespace job::usb {

JobUsbMonitor::JobUsbMonitor()
{
    m_udev = udev_new();

    if (!m_udev)
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to create udev context.");
}

JobUsbMonitor::JobUsbMonitor(const threads::JobIoAsyncThread::Ptr &loop) :
    m_loop(loop)
{
    m_udev = udev_new();

    if (!m_udev)
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to create udev context.");
}

JobUsbMonitor::~JobUsbMonitor()
{
    stop();

    if (m_udev)
        udev_unref(m_udev);
}

JobUsbMonitor::Ptr JobUsbMonitor::createShared()
{
    return std::make_shared<JobUsbMonitor>();
}

JobUsbMonitor::Ptr JobUsbMonitor::createShared(const threads::JobIoAsyncThread::Ptr &loop)
{
    return std::make_shared<JobUsbMonitor>(loop);
}

JobUsbMonitor::UPtr JobUsbMonitor::createUniq()
{
    return std::make_unique<JobUsbMonitor>();
}

JobUsbMonitor::UPtr JobUsbMonitor::createUniq(const threads::JobIoAsyncThread::Ptr &loop)
{
    return std::make_unique<JobUsbMonitor>(loop);
}

bool JobUsbMonitor::start(Callback callback)
{
    if (m_running)
        return false;

    if (!m_udev) {
        JOB_LOG_ERROR("[JobUsbMonitor] udev context is not available.");
        return false;
    }

    auto loop = m_loop.lock();

    if (!loop || !loop->isRunning()) {
        JOB_LOG_ERROR("[JobUsbMonitor] Event loop is not valid or not running.");
        return false;
    }

    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");

    if (!m_monitor) {
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to create udev monitor.");
        return false;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "usb", "usb_device") < 0) {
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to add USB device filter.");

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    if (udev_monitor_enable_receiving(m_monitor) < 0) {
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to enable udev monitor.");

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    m_fd = udev_monitor_get_fd(m_monitor);

    if (m_fd < 0) {
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to get udev monitor FD.");

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    m_callback = std::move(callback);
    const auto events = threads::IOEvent::Read |
                        threads::IOEvent::Error |
                        threads::IOEvent::HangUp |
                        threads::IOEvent::EdgeTriggered;

    if (!loop->registerFD(m_fd, events, [this](threads::IOEvent event) {
            onEvents(event);
        })) {
        JOB_LOG_ERROR("[JobUsbMonitor] Failed to register FD {} with event loop.", m_fd);

        m_callback = nullptr;
        m_fd = -1;

        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;

        return false;
    }

    m_running = true;

    return true;
}

bool JobUsbMonitor::start(const threads::JobIoAsyncThread::Ptr &loop, Callback callback)
{
    if (!loop)
        return false;

    if (m_running)
        return false;

    m_loop = loop;

    return start(std::move(callback));
}

void JobUsbMonitor::stop()
{
    if (m_fd != -1) {
        if (auto loop = m_loop.lock()) {
            if (!loop->unregisterFD(m_fd))
                JOB_LOG_WARN("[JobUsbMonitor] Failed to unregister FD {}.", m_fd);
        }

        m_fd = -1;
    }

    if (m_monitor) {
        udev_monitor_unref(m_monitor);
        m_monitor = nullptr;
    }

    m_callback = nullptr;
    m_running = false;
}

bool JobUsbMonitor::isRunning() const noexcept
{
    return m_running;
}

void JobUsbMonitor::onEvents(threads::IOEvent events)
{
    if (threads::hasEvent(events, threads::IOEvent::Error) || threads::hasEvent(events, threads::IOEvent::HangUp)) {
        JOB_LOG_ERROR("[JobUsbMonitor] Error or hangup on udev monitor FD.");
        stop();
        return;
    }

    if (threads::hasEvent(events, threads::IOEvent::Read))
        drain();
}

void JobUsbMonitor::drain()
{
    if (!m_monitor)
        return;

    while (udev_device *device = udev_monitor_receive_device(m_monitor)) {
        JobUsbMonitorAction action = JobUsbMonitorAction::Unknown;


        if (const char *nativeAction = udev_device_get_action(device)) {
            JOB_LOG_INFO("[JobUsbMonitor] udev action: {}", nativeAction);
            if (std::strcmp(nativeAction, "add") == 0)
                action = JobUsbMonitorAction::Added;
            else if (std::strcmp(nativeAction, "remove") == 0)
                action = JobUsbMonitorAction::Removed;
            else if (std::strcmp(nativeAction, "change") == 0)
                action = JobUsbMonitorAction::Changed;
            else if (std::strcmp(nativeAction, "bind") == 0)
                action = JobUsbMonitorAction::Bound;
            else if (std::strcmp(nativeAction, "unbind") == 0)
                action = JobUsbMonitorAction::Unbound;
        }

        const char *nativePath = udev_device_get_syspath(device);
        if (m_callback) {
            const std::string_view path = nativePath ? std::string_view{nativePath} : std::string_view{};
            m_callback(action, path);
        }

        udev_device_unref(device);
    }
}

} // namespace job::usb