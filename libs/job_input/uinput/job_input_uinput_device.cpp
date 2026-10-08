#include "job_input_uinput_device.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <utility>

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace job::input {

JobInputUInputDevice::JobInputUInputDevice(JobInputDevice device, fs::path path) :
    m_device(std::move(device)),
    m_path(std::move(path))
{

}

JobInputUInputDevice::~JobInputUInputDevice()
{
    reset();
}

JobInputUInputDevice::JobInputUInputDevice(JobInputUInputDevice &&other) noexcept
{
    moveFrom(std::move(other));
}

JobInputUInputDevice &JobInputUInputDevice::operator=(JobInputUInputDevice &&other) noexcept
{
    if (this != &other) {
        reset();
        moveFrom(std::move(other));
    }

    return *this;
}

JobInputUInputDevice::Ptr JobInputUInputDevice::createShared(JobInputDevice device, fs::path path)
{
    return std::make_shared<JobInputUInputDevice>(std::move(device), std::move(path));
}

JobInputUInputDevice::UPtr JobInputUInputDevice::createUniq(JobInputDevice device, fs::path path)
{
    return std::make_unique<JobInputUInputDevice>(std::move(device), std::move(path));
}

bool JobInputUInputDevice::create()
{
    if (m_created)
        return true;

    if (!openDevice())
        return false;

    if (!configureCapabilities()) {
        closeDevice();
        return false;
    }

    if (!configureIdentity()) {
        closeDevice();
        return false;
    }

    if (!createKernelDevice()) {
        closeDevice();
        return false;
    }

    m_created = true;
    return true;
}

void JobInputUInputDevice::destroy()
{
    if (m_fd < 0)
        return;

    if (m_created)
        ::ioctl(m_fd, UI_DEV_DESTROY);

    m_created = false;
    closeDevice();
}

bool JobInputUInputDevice::send(const JobInputEvent &event)
{
    return send(event.type(), event.code(), event.value());
}

bool JobInputUInputDevice::sync()
{
    return send(static_cast<JobInputType>(EV_SYN), static_cast<JobInputCode>(SYN_REPORT), static_cast<JobInputValue>(0));
}

bool JobInputUInputDevice::isCreated() const noexcept
{
    return m_created;
}

int JobInputUInputDevice::fd() const noexcept
{
    return m_fd;
}

const JobInputDevice &JobInputUInputDevice::device() const noexcept
{
    return m_device;
}

const fs::path &JobInputUInputDevice::path() const noexcept
{
    return m_path;
}

bool JobInputUInputDevice::openDevice()
{
    if (m_fd >= 0)
        return true;

    m_fd = ::open(m_path.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC);

    return m_fd >= 0;
}

void JobInputUInputDevice::closeDevice() noexcept
{
    if (m_fd >= 0)
        ::close(m_fd);

    m_fd = -1;
}

bool JobInputUInputDevice::configureCapabilities()
{
    if (m_fd < 0)
        return false;

    const JobInputCapability &capability = m_device.capability();

    const auto setBits = [this](unsigned long request, const auto &bits) {
        for (std::size_t index = 0; index < bits.size(); ++index) {
            if (!bits.test(index))
                continue;

            if (::ioctl(m_fd, request, static_cast<int>(index)) < 0)
                return false;
        }

        return true;
    };

    if (!setBits(UI_SET_PROPBIT, capability.deviceProperties()))
        return false;

    if (!setBits(UI_SET_EVBIT, capability.eventTypes()))
        return false;

    if (!setBits(UI_SET_KEYBIT, capability.keys()))
        return false;

    if (!setBits(UI_SET_RELBIT, capability.relativeAxes()))
        return false;

    if (!setBits(UI_SET_ABSBIT, capability.absoluteAxes()))
        return false;

    if (!setBits(UI_SET_SWBIT, capability.switches()))
        return false;

    if (!setBits(UI_SET_MSCBIT, capability.miscEvents()))
        return false;

    if (!setBits(UI_SET_LEDBIT, capability.leds()))
        return false;

    if (!setBits(UI_SET_SNDBIT, capability.sounds()))
        return false;

    if (!setBits(UI_SET_FFBIT, capability.forceFeedback()))
        return false;

    return true;
}

bool JobInputUInputDevice::configureIdentity()
{
    if (m_fd < 0)
        return false;

    const std::string_view physicalLocation = m_device.physicalLocation();

    if (!physicalLocation.empty()) {
        const std::string physicalLocationString(physicalLocation);

        if (::ioctl(m_fd, UI_SET_PHYS, physicalLocationString.c_str()) < 0)
            return false;
    }

    uinput_setup setup{};

    setup.id.bustype = m_device.busType();
    setup.id.vendor = m_device.vendor();
    setup.id.product = m_device.product();
    setup.id.version = m_device.version();

    const std::string_view name = m_device.name();
    const std::size_t nameLength = std::min(name.size(), sizeof(setup.name) - 1);

    std::memcpy(setup.name, name.data(), nameLength);
    setup.name[nameLength] = '\0';

    setup.ff_effects_max = 0;

    return ::ioctl(m_fd, UI_DEV_SETUP, &setup) == 0;
}

bool JobInputUInputDevice::createKernelDevice()
{
    if (m_fd < 0)
        return false;

    return ::ioctl(m_fd, UI_DEV_CREATE) == 0;
}

bool JobInputUInputDevice::send(JobInputType type, JobInputCode code, JobInputValue value)
{
    if (!m_created || m_fd < 0)
        return false;

    input_event event{};

    event.type  = static_cast<decltype(event.type)>(type);
    event.code  = static_cast<decltype(event.code)>(code);
    event.value = static_cast<decltype(event.value)>(value);

    const ssize_t bytesWritten = ::write(m_fd, &event, sizeof(event));

    return bytesWritten == static_cast<ssize_t>(sizeof(event));
}

void JobInputUInputDevice::reset() noexcept
{
    destroy();
}

void JobInputUInputDevice::moveFrom(JobInputUInputDevice &&other) noexcept
{
    m_device = std::move(other.m_device);
    m_path = std::move(other.m_path);

    m_fd = other.m_fd;
    m_created = other.m_created;

    other.m_fd = -1;
    other.m_created = false;
}

} // namespace job::input