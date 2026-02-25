#pragma once

#include <map>
#include <string>
#include <memory>

#include "serial_io.h"
#include "udev_monitor_thread.h"
#include <job_io_async_thread.h>
#include <job_logger.h>

namespace job::uart {

class SerialManager
{
public:
    SerialManager();
    ~SerialManager();

    SerialManager(const SerialManager &) = delete;
    SerialManager &operator=(const SerialManager &) = delete;

    void scanDevices();

    [[nodiscard]] const std::map<std::string, std::unique_ptr<SerialIO>> &devices() const noexcept;

    [[nodiscard]] SerialIO *currentDevice() const noexcept;
    void setCurrentDevice(const std::string &path);

    [[nodiscard]] std::shared_ptr<threads::JobIoAsyncThread> loop() const;

private:
    std::map<std::string, std::unique_ptr<SerialIO>> m_devices;
    SerialIO *m_currentDevice{nullptr};

    std::shared_ptr<threads::JobIoAsyncThread> m_loop;
    std::unique_ptr<UdevMonitorThread> m_monitor;
};

} // namespace job::uart

