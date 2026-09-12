#include "job_usb_parser.h"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include <libusb-1.0/libusb.h>

#include "job_usb_id_database.h"
#include "job_usb_udev.h"

namespace job::usb {

JobUsbDevice::Ptr JobUsbParser::parse(libusb_device *device,
                                      const libusb_device_descriptor &descriptor,
                                      const JobUsbIdDatabase *database,
                                      const JobUsbUdev *udev)
{
    if (!device)
        return {};

    JobUsbDescriptor jobDescriptor{
        descriptor.idVendor,
        descriptor.idProduct,
        descriptor.bcdUSB,
        descriptor.bcdDevice,
        descriptor.bDeviceClass,
        descriptor.bDeviceSubClass,
        descriptor.bDeviceProtocol
    };

    const uint8_t busNumber = libusb_get_bus_number(device);
    const uint8_t deviceAddress = libusb_get_device_address(device);

    JobUsbLocation::PortPath portPath;
    std::array<uint8_t, 7> ports{};

    const int portCount = libusb_get_port_numbers(device,
                                                  ports.data(),
                                                  static_cast<int>(ports.size()));

    if (portCount > 0) {
        for (int i = 0; i < portCount; ++i)
            portPath.push_back(ports[static_cast<std::size_t>(i)]);
    }

    std::optional<std::string> path;

    if (udev)
        path = udev->sysfsPath(busNumber, deviceAddress);

    JobUsbLocation location{
        busNumber,
        deviceAddress,
        std::move(portPath),
        std::move(path)
    };

    JobUsbStrings strings;

    struct HandleDeleter
    {
        void operator()(libusb_device_handle *handle) const noexcept
        {
            if (handle)
                libusb_close(handle);
        }
    };

    using HandlePtr = std::unique_ptr<libusb_device_handle, HandleDeleter>;

    libusb_device_handle *nativeHandle = nullptr;

    if (libusb_open(device, &nativeHandle) == LIBUSB_SUCCESS && nativeHandle) {
        HandlePtr handle{nativeHandle};

        const auto readString = [&](uint8_t index) -> std::optional<std::string> {
            if (index == 0)
                return std::nullopt;

            std::array<unsigned char, 256> buffer{};

            const int length = libusb_get_string_descriptor_ascii(handle.get(),
                                                                  index,
                                                                  buffer.data(),
                                                                  static_cast<int>(buffer.size()));

            if (length <= 0)
                return std::nullopt;

            return std::string{
                reinterpret_cast<const char *>(buffer.data()),
                static_cast<std::size_t>(length)
            };
        };

        strings.setManufacturer(readString(descriptor.iManufacturer));
        strings.setProduct(readString(descriptor.iProduct));
        strings.setSerialNumber(readString(descriptor.iSerialNumber));
    }

    if (database && database->ready()) {
        if (!strings.manufacturer()) {
            const std::string_view vendor = database->vendorName(descriptor.idVendor);

            if (!vendor.empty())
                strings.setManufacturer(std::string{vendor});
        }

        if (!strings.product()) {
            const std::string_view product = database->productName(descriptor.idVendor,
                                                                   descriptor.idProduct);

            if (!product.empty())
                strings.setProduct(std::string{product});
        }
    }

    JobUsbState state{
        device,
        true
    };

    return JobUsbDevice::createShared(0,
                                      std::move(jobDescriptor),
                                      std::move(location),
                                      std::move(strings),
                                      std::move(state));
}

} // namespace job::usb