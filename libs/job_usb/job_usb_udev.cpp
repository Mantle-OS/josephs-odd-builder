#include "job_usb_udev.h"

#include <charconv>

#include <libudev.h>

namespace job::usb {

JobUsbUdev::JobUsbUdev() :
    m_udev(udev_new())
{
}

JobUsbUdev::Ptr JobUsbUdev::createShared()
{
    return std::make_shared<JobUsbUdev>();
}

JobUsbUdev::UPtr JobUsbUdev::createUniq()
{
    return std::make_unique<JobUsbUdev>();
}

bool JobUsbUdev::valid() const noexcept
{
    return m_udev != nullptr;
}

std::optional<std::string> JobUsbUdev::sysfsPath(uint8_t busNumber,
                                                 uint8_t deviceAddress) const
{
    const std::lock_guard lock{m_mutex};

    auto device = findDevice(busNumber, deviceAddress);

    if (!device)
        return std::nullopt;

    const char *path = udev_device_get_syspath(device.get());

    if (!path)
        return std::nullopt;

    return std::string{path};
}

std::optional<std::string> JobUsbUdev::property(uint8_t busNumber,
                                                uint8_t deviceAddress,
                                                std::string_view name) const
{
    const std::lock_guard lock{m_mutex};

    auto device = findDevice(busNumber, deviceAddress);

    if (!device)
        return std::nullopt;

    const std::string key{name};

    const char *value = udev_device_get_property_value(device.get(), key.c_str());

    if (!value)
        return std::nullopt;

    return std::string{value};
}

void JobUsbUdev::UdevDeleter::operator()(udev *context) const noexcept
{
    if (context)
        udev_unref(context);
}

void JobUsbUdev::UdevDeviceDeleter::operator()(udev_device *device) const noexcept
{
    if (device)
        udev_device_unref(device);
}


bool JobUsbUdev::parseUnsigned(std::string_view text, unsigned int &value) noexcept
{
    if (text.empty())
        return false;

    const auto result = std::from_chars(text.data(),
                                        text.data() + text.size(),
                                        value);

    return result.ec == std::errc{} &&
           result.ptr == text.data() + text.size();
}

JobUsbUdev::UdevDevicePtr JobUsbUdev::findDevice(uint8_t busNumber,
                                                 uint8_t deviceAddress) const
{
    if (!m_udev)
        return {};

    auto enumerate = std::unique_ptr<udev_enumerate, decltype(&udev_enumerate_unref)>{
        udev_enumerate_new(m_udev.get()),
        &udev_enumerate_unref
    };

    if (!enumerate)
        return {};

    if (udev_enumerate_add_match_subsystem(enumerate.get(), "usb") < 0)
        return {};

    if (udev_enumerate_add_match_property(enumerate.get(), "DEVTYPE", "usb_device") < 0)
        return {};

    if (udev_enumerate_scan_devices(enumerate.get()) < 0)
        return {};

    udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate.get());
    udev_list_entry *entry = nullptr;
    udev_list_entry_foreach(entry, devices) {
        const char *syspath = udev_list_entry_get_name(entry);

        if (!syspath)
            continue;

        UdevDevicePtr device{ udev_device_new_from_syspath(m_udev.get(), syspath) };
        if (!device)
            continue;

        const char *busText = udev_device_get_sysattr_value(device.get(), "busnum");
        const char *addressText = udev_device_get_sysattr_value(device.get(), "devnum");
        if (!busText || !addressText)
            continue;

        unsigned int bus = 0;
        unsigned int address = 0;

        if (!parseUnsigned(busText, bus))
            continue;

        if (!parseUnsigned(addressText, address))
            continue;

        if (bus != busNumber || address != deviceAddress)
            continue;

        return device;
    }

    return {};
}

} // namespace job::usb