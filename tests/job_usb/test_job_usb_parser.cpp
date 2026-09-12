#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

#include <libusb-1.0/libusb.h>

#include <job_usb_id_database.h>
#include <job_usb_parser.h>

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: Deterministic behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbParser rejects a null libusb device",
          "[job_usb][parser][null]")
{
    libusb_device_descriptor descriptor{};
    const auto device = JobUsbParser::parse(nullptr, descriptor);
    REQUIRE_FALSE(device);
}

//////////////////////////////////////////////////////////
// Block 2: Local libusb integration
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbParser parses real libusb devices",
          "[job_usb][parser][local][integration]")
{
    struct ContextDeleter
    {
        void operator()(libusb_context *context) const noexcept
        {
            if (context)
                libusb_exit(context);
        }
    };

    struct DeviceListDeleter
    {
        void operator()(libusb_device **devices) const noexcept
        {
            if (devices)
                libusb_free_device_list(devices, 1);
        }
    };

    using ContextPtr = std::unique_ptr<libusb_context, ContextDeleter>;
    using DeviceListPtr = std::unique_ptr<libusb_device *, DeviceListDeleter>;

    libusb_context *nativeContext = nullptr;

    REQUIRE(libusb_init(&nativeContext) == LIBUSB_SUCCESS);
    REQUIRE(nativeContext != nullptr);

    ContextPtr context{nativeContext};
    libusb_device **nativeDevices = nullptr;

    const ssize_t deviceCount = libusb_get_device_list(context.get(), &nativeDevices);
    REQUIRE(deviceCount >= 0);
    REQUIRE(nativeDevices != nullptr);

    DeviceListPtr devices{nativeDevices};

    auto &database = JobUsbIdDatabase::instance();

    database.clear();

    REQUIRE(database.load("/usr/share/hwdata/usb.ids"));
    REQUIRE(database.ready());

    std::size_t parsedCount = 0;

    for (ssize_t i = 0; i < deviceCount; ++i) {
        libusb_device *nativeDevice = nativeDevices[i];

        REQUIRE(nativeDevice != nullptr);

        libusb_device_descriptor nativeDescriptor{};

        REQUIRE(libusb_get_device_descriptor(nativeDevice, &nativeDescriptor) == LIBUSB_SUCCESS);

        const auto device = JobUsbParser::parse(nativeDevice, nativeDescriptor, &database);
        REQUIRE(device);

        const auto &descriptor = device->descriptor();

        CAPTURE(nativeDescriptor.idVendor,
                nativeDescriptor.idProduct,
                nativeDescriptor.bDeviceClass,
                nativeDescriptor.bDeviceSubClass,
                nativeDescriptor.bDeviceProtocol);

        REQUIRE(descriptor.vendorId() == nativeDescriptor.idVendor);
        REQUIRE(descriptor.productId() == nativeDescriptor.idProduct);
        REQUIRE(descriptor.usbSpecVersion() == nativeDescriptor.bcdUSB);
        REQUIRE(descriptor.deviceVersion() == nativeDescriptor.bcdDevice);
        REQUIRE(descriptor.deviceClass() == nativeDescriptor.bDeviceClass);
        REQUIRE(descriptor.subClass() == nativeDescriptor.bDeviceSubClass);
        REQUIRE(descriptor.protocol() == nativeDescriptor.bDeviceProtocol);

        const auto &location = device->location();

        REQUIRE(location.busNumber() == libusb_get_bus_number(nativeDevice));
        REQUIRE(location.deviceAddress() == libusb_get_device_address(nativeDevice));
        REQUIRE(location.portPath().size() <= 7);

        ++parsedCount;
    }

    REQUIRE(parsedCount == static_cast<std::size_t>(deviceCount));

    database.clear();
}

} // namespace job::usb::tests