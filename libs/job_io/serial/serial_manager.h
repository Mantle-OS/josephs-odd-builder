#pragma once

#include <map>
#include <string>
#include <memory>

#include "serial_io.h"
#include "udev_monitor_thread.h"

namespace job::io {

class SerialManager
{
public:
    SerialManager();
    ~SerialManager();

    void scanDevices();

    [[nodiscard]] const std::map<std::string, std::unique_ptr<SerialIO>> &devices() const noexcept;

    [[nodiscard]] SerialIO *currentDevice() const noexcept;
    void setCurrentDevice(const std::string &path);

private:
    std::map<std::string, std::unique_ptr<SerialIO>> m_devices;
    SerialIO *m_currentDevice{nullptr};

    std::unique_ptr<UdevMonitorThread> m_monitor;
};

} // namespace job::io
