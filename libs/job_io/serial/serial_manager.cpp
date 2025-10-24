#include "serial_manager.h"
#include "serial_info.h"

#include <iostream>

namespace job::io {

SerialManager::SerialManager()
{
    scanDevices();  // Initial scan

    // Start udev monitor to track hotplug events
    m_monitor = std::make_unique<UdevMonitorThread>();
    m_monitor->start([this]() {
        std::cout << "[SerialManager] Device change detected, rescanning...\n";
        scanDevices();
    });
}

SerialManager::~SerialManager()
{
    if (m_monitor)
        m_monitor->stop();

    // All cleanup handled by unique_ptr
    m_devices.clear();
    m_currentDevice = nullptr;
}

void SerialManager::scanDevices()
{
    serial_info::update_serial_devices(m_devices);

    if (!m_currentDevice && !m_devices.empty())
        m_currentDevice = m_devices.begin()->second.get();
}

const std::map<std::string, std::unique_ptr<SerialIO>> &SerialManager::devices() const noexcept
{
    return m_devices;
}

SerialIO *SerialManager::currentDevice() const noexcept
{
    return m_currentDevice;
}

void SerialManager::setCurrentDevice(const std::string &path)
{
    auto it = m_devices.find(path);
    if (it != m_devices.end())
        m_currentDevice = it->second.get();
}

} // namespace job::io
