#include "udev_monitor_thread.h"
#include <poll.h>
#include <unistd.h>
#include <cstring>

namespace job::uart {

UdevMonitorThread::UdevMonitorThread()
    : m_udev(udev_new())
{
}

UdevMonitorThread::~UdevMonitorThread()
{
    stop();
    if (m_monitor)
        udev_monitor_unref(m_monitor);
    if (m_udev)
        udev_unref(m_udev);
}

void UdevMonitorThread::start(const Callback &onDeviceChange)
{
    if (m_running.exchange(true))
        return;

    m_callback = onDeviceChange;

    threads::JobThreadOptions opts = threads::JobThreadOptions::normal();
    opts.name = "UdevMonitor";

    m_thread = std::make_shared<threads::JobThread>(opts);
    m_thread->setRunFunction([this](std::stop_token token) {
        this->monitorLoop(token);
    });

    const auto result = m_thread->start();
    if (result != threads::JobThread::StartResult::Started)
        m_running = false;
}

void UdevMonitorThread::stop()
{
    if (!m_running.exchange(false))
        return;

    if (m_thread) {
        m_thread->requestStop();
        m_thread->join();
        m_thread.reset();
    }
}

void UdevMonitorThread::monitorLoop(std::stop_token token)
{
    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_monitor)
        return;

    udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "tty", nullptr);
    udev_monitor_enable_receiving(m_monitor);

    const int fd = udev_monitor_get_fd(m_monitor);
    struct pollfd fds { fd, POLLIN, 0 };

    while (!token.stop_requested()) {
        const int ret = ::poll(&fds, 1, 1000); // 1s timeout
        if (ret <= 0)
            continue;

        if (fds.revents & POLLIN) {
            if (struct udev_device *dev = udev_monitor_receive_device(m_monitor)) {
                const char *action = udev_device_get_action(dev);
                if (action && (strcmp(action, "add") == 0 || strcmp(action, "remove") == 0)) {
                    if (m_callback)
                        m_callback();
                }
                udev_device_unref(dev);
            }
        }
    }
}

} // namespace job::uart
