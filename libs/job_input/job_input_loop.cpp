#include "job_input_loop.h"

#include <algorithm>
#include <utility>
#include <job_logger.h>

namespace job::input {

JobInputLoop::JobInputLoop(threads::JobIoAsyncThread::Ptr loop) :
    m_loop(std::move(loop))
{
}

JobInputLoop::~JobInputLoop()
{
    clear();
}

bool JobInputLoop::addDevice(const JobInputLinuxDevice::Ptr &device)
{
    if (!m_loop || !device || !device->isOpen())
        return false;

    if (findDevice(device) != m_devices.end())
        return true;

    const JobInputDevice *inputDevice = device->device();
    if (!inputDevice)
        return false;

    const WPtr weakSelf = weak_from_this();
    if (weakSelf.expired())
        return false;

    auto reader = JobInputLinuxReader::createShared(device);
    if (!reader)
        return false;

    auto dispatcher = JobInputDispatcher::createShared(inputDevice->classification());
    if (!dispatcher)
        return false;

    const JobInputDispatcher::WPtr weakDispatcher = dispatcher;

    reader->setEventCallback([weakDispatcher](const JobInputEvent &event) {
        if (const auto dispatcher = weakDispatcher.lock())
            dispatcher->dispatch(event);
    });

    m_devices.push_back(DeviceContext{
        .device = device,
        .reader = std::move(reader),
        .dispatcher = std::move(dispatcher)
    });

    const JobInputLinuxDevice::WPtr weakDevice = device;

    const bool registered = m_loop->registerFD(device->fd(),
                                               threads::IOEvent::Read | threads::IOEvent::Error | threads::IOEvent::HangUp,
                                               [weakSelf, weakDevice](threads::IOEvent events) {
        const auto self = weakSelf.lock();
        const auto device = weakDevice.lock();

        if (!self || !device)
            return;

        if (threads::hasEvent(events, threads::IOEvent::Error) ||
            threads::hasEvent(events, threads::IOEvent::HangUp)) {
            self->removeDevice(device);
            return;
        }

        if (threads::hasEvent(events, threads::IOEvent::Read))
            self->handleReadable(device);
    });

    if (!registered) {
        const auto it = findDevice(device);
        if (it != m_devices.end())
            m_devices.erase(it);

        return false;
    }

    return true;
}

bool JobInputLoop::removeDevice(const JobInputLinuxDevice::Ptr &device)
{
    if (!device)
        return false;

    const auto it = findDevice(device);
    if (it == m_devices.end())
        return false;

    const int fd = it->device ? it->device->fd() : -1;

    if (m_loop && fd >= 0) {
        if (!m_loop->unregisterFD(fd))
            JOB_LOG_WARN("[JobInputLoop] Failed to unregister input fd {}.", fd);
    }

    m_devices.erase(it);
    return true;
}

void JobInputLoop::clear()
{
    if (m_loop) {
        for (const auto &context : m_devices) {
            if (!context.device)
                continue;

            const int fd = context.device->fd();
            if (fd < 0)
                continue;

            if (!m_loop->unregisterFD(fd))
                JOB_LOG_WARN("[JobInputLoop] Failed to unregister input fd {} during clear.", fd);
        }
    }

    m_devices.clear();
}

void JobInputLoop::handleReadable(const JobInputLinuxDevice::Ptr &device)
{
    const auto it = findDevice(device);
    if (it == m_devices.end() || !it->reader)
        return;

    if (!it->reader->readEvents())
        removeDevice(device);
}

JobInputLoop::DeviceContexts::iterator JobInputLoop::findDevice(const JobInputLinuxDevice::Ptr &device)
{
    return std::find_if(m_devices.begin(), m_devices.end(), [&device](const DeviceContext &context) {
        return context.device == device;
    });
}

JobInputLoop::DeviceContexts::const_iterator JobInputLoop::findDevice(const JobInputLinuxDevice::Ptr &device) const
{
    return std::find_if(m_devices.cbegin(), m_devices.cend(), [&device](const DeviceContext &context) {
        return context.device == device;
    });
}

} // namespace job::input