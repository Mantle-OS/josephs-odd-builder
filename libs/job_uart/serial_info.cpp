#include "serial_info.h"
#include "serial_io.h"

#include <filesystem>
#include <regex>
#include <iostream>
#include <libudev.h>

namespace job::uart::serial_info {

namespace {
// Build regex dynamically from known prefixes
const std::regex kSerialPattern(
    R"((ttyS|ttyUSB|ttyACM|ttyAMA|ttyGS|ttyMI|ttymxc|ttyTHS|ttyO|rfcomm|ircomm|tnt)\d+)"
    );

[[nodiscard]] bool matchesSerialPattern(const std::string &name)
{
    std::string base_name = std::filesystem::path(name).filename().string();
    return std::regex_match(base_name, kSerialPattern);}
}

std::vector<std::string> scanSerialPortsFromFilesystem()
{
    std::vector<std::string> ports;
    for (const auto &entry : std::filesystem::directory_iterator("/dev")) {
        const std::string name = entry.path().filename();
        if (matchesSerialPattern(name))
            ports.emplace_back("/dev/" + name);
    }
    return ports;
}

void update_serial_devices(
    std::map<std::string, std::unique_ptr<SerialIO>> &deviceMap,
    std::shared_ptr<threads::JobIoAsyncThread> loop)
{
    struct udev *udev = ::udev_new();
    if (!udev) {
        JOB_LOG_ERROR("[serial_info] Failed to create udev context.");
        return;
    }

    struct udev_enumerate *enumerate = ::udev_enumerate_new(udev);
    if (!enumerate) {
        std::cerr << "[serial_info] Failed to create udev enumerator.\n";
        ::udev_unref(udev);
        return;
    }

    ::udev_enumerate_add_match_subsystem(enumerate, "tty");
    ::udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = ::udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry = nullptr;

    udev_list_entry_foreach(entry, devices) {
        const char *sysPath = ::udev_list_entry_get_name(entry);
        struct udev_device *dev = ::udev_device_new_from_syspath(udev, sysPath);
        if (!dev)
            continue;

        const char *node = ::udev_device_get_devnode(dev);
        if (!node) {
            ::udev_device_unref(dev);
            continue;
        }

        std::string path(node);
        if (!matchesSerialPattern(path)) {
            ::udev_device_unref(dev);
            continue;
        }

        auto &portPtr = deviceMap[path];
        if (!portPtr)
            portPtr = std::make_unique<SerialIO>(loop);

        portPtr->setLocation(path);

        auto set_prop = [&](const char* key, auto setter) {
            const char* val = ::udev_device_get_property_value(dev, key);
            if (val) setter(val);
        };

        set_prop("ID_MODEL", [&](const char* v){ portPtr->setDescription(v); });
        set_prop("ID_VENDOR", [&](const char* v){ portPtr->setManufacturer(v); });
        set_prop("ID_SERIAL_SHORT", [&](const char* v){ portPtr->setSerialNumber(v); });

        if (const char *vendorId = ::udev_device_get_property_value(dev, "ID_VENDOR_ID"))
            portPtr->setVendorId(static_cast<uint16_t>(std::strtol(vendorId, nullptr, 16)));

        if (const char *productId = ::udev_device_get_property_value(dev, "ID_MODEL_ID"))
            portPtr->setProductId(static_cast<uint16_t>(std::strtol(productId, nullptr, 16)));

        ::udev_device_unref(dev);
    }

    ::udev_enumerate_unref(enumerate);
    ::udev_unref(udev);
}

} // namespace job::io::serial_info

