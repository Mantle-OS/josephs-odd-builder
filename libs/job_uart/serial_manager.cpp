#include "serial_manager.h"
#include "serial_info.h"

namespace job::uart {

SerialManager::SerialManager()
{
    try {
        m_loop = std::make_shared<threads::JobIoAsyncThread>();
        m_loop->start(); // Start the loop thread immediately
    } catch (const std::exception& e) {
        JOB_LOG_ERROR("[SerialManager] Failed to create JobIoAsyncThread: {}", e.what());
        return; // This is a fatal error
    }
    scanDevices();
    m_monitor = std::make_unique<UdevMonitorThread>();
    if (!m_monitor->start(m_loop, [this]() {
            JOB_LOG_INFO("[SerialManager] Device change detected, rescanning...");
            scanDevices();
        }))
        JOB_LOG_ERROR("[SerialManager] Failed to start UdevMonitorThread!");
}

SerialManager::~SerialManager()
{
    if (m_monitor)
        m_monitor->stop();

    for (auto& [path, device] : m_devices)
        if (device->isOpen())
            device->closeDevice();

    if (m_loop)
        m_loop->stop();

    m_devices.clear();
    m_currentDevice = nullptr;
}

void SerialManager::scanDevices()
{
    serial_info::update_serial_devices(m_devices, m_loop);

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

std::shared_ptr<threads::JobIoAsyncThread> SerialManager::loop() const
{
    return m_loop;
}

} // namespace job::uart
// CHECKPOINT: v2.0
