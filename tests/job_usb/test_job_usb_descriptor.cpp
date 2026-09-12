#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <job_usb_descriptor.h>

using namespace job::usb;
namespace job::usb::tests {

// =============================================================================
// Block 1: Usage / examples
// =============================================================================
TEST_CASE("JobUsbDescriptor describes a USB device descriptor",
          "[usb][descriptor][usage]")
{
    JobUsbDescriptor descriptor{
        0x0955,
        0x7321,
        0x0300,
        0x0102,
        0xEF,
        0x02,
        0x01
    };

    REQUIRE(descriptor.vendorId() == 0x0955);
    REQUIRE(descriptor.productId() == 0x7321);
    REQUIRE(descriptor.usbSpecVersion() == 0x0300);
    REQUIRE(descriptor.deviceVersion() == 0x0102);
    REQUIRE(descriptor.deviceClass() == 0xEF);
    REQUIRE(descriptor.subClass() == 0x02);
    REQUIRE(descriptor.protocol() == 0x01);
}

TEST_CASE("JobUsbDescriptor fields can be updated after construction",
          "[usb][descriptor][usage]")
{
    JobUsbDescriptor descriptor;

    descriptor.setVendorId(0x1234);
    descriptor.setProductId(0x5678);
    descriptor.setUsbSpecVersion(0x0200);
    descriptor.setDeviceVersion(0x0100);
    descriptor.setDeviceClass(0xFF);
    descriptor.setSubClass(0x42);
    descriptor.setProtocol(0x07);

    REQUIRE(descriptor.vendorId() == 0x1234);
    REQUIRE(descriptor.productId() == 0x5678);
    REQUIRE(descriptor.usbSpecVersion() == 0x0200);
    REQUIRE(descriptor.deviceVersion() == 0x0100);
    REQUIRE(descriptor.deviceClass() == 0xFF);
    REQUIRE(descriptor.subClass() == 0x42);
    REQUIRE(descriptor.protocol() == 0x07);
}

TEST_CASE("JobUsbDescriptor factories construct populated descriptors",
          "[usb][descriptor][factory]")
{
    auto shared = JobUsbDescriptor::createShared(
        0x1D6B,
        0x0002,
        0x0200,
        0x0601,
        0x09,
        0x00,
        0x01
        );

    REQUIRE(shared);
    REQUIRE(shared->vendorId() == 0x1D6B);
    REQUIRE(shared->productId() == 0x0002);
    REQUIRE(shared->usbSpecVersion() == 0x0200);
    REQUIRE(shared->deviceVersion() == 0x0601);
    REQUIRE(shared->deviceClass() == 0x09);
    REQUIRE(shared->subClass() == 0x00);
    REQUIRE(shared->protocol() == 0x01);

    auto unique = JobUsbDescriptor::createUniq(
        0x0403,
        0x6001,
        0x0200,
        0x0600,
        0x00,
        0x00,
        0x00
        );

    REQUIRE(unique);
    REQUIRE(unique->vendorId() == 0x0403);
    REQUIRE(unique->productId() == 0x6001);
}

TEST_CASE("JobUsbDescriptor survives JOB serialization round trips",
          "[usb][descriptor][serialization]")
{
    JobUsbDescriptor source{
        0x0955,
        0x7321,
        0x0300,
        0x0102,
        0xEF,
        0x02,
        0x01
    };

    SECTION("JSON")
    {
        std::string data;
        REQUIRE(source.toJobJson(data));

        JobUsbDescriptor restored;
        REQUIRE(restored.fromJobJson(data));

        REQUIRE(restored.vendorId() == source.vendorId());
        REQUIRE(restored.productId() == source.productId());
        REQUIRE(restored.usbSpecVersion() == source.usbSpecVersion());
        REQUIRE(restored.deviceVersion() == source.deviceVersion());
        REQUIRE(restored.deviceClass() == source.deviceClass());
        REQUIRE(restored.subClass() == source.subClass());
        REQUIRE(restored.protocol() == source.protocol());
    }

    SECTION("YAML")
    {
        std::string data;
        REQUIRE(source.toJobYaml(data));

        JobUsbDescriptor restored;
        REQUIRE(restored.fromJobYaml(data));

        REQUIRE(restored.vendorId() == source.vendorId());
        REQUIRE(restored.productId() == source.productId());
        REQUIRE(restored.usbSpecVersion() == source.usbSpecVersion());
        REQUIRE(restored.deviceVersion() == source.deviceVersion());
        REQUIRE(restored.deviceClass() == source.deviceClass());
        REQUIRE(restored.subClass() == source.subClass());
        REQUIRE(restored.protocol() == source.protocol());
    }

    SECTION("binary")
    {
        std::vector<std::uint8_t> data;
        REQUIRE(source.toBinary(data));

        JobUsbDescriptor restored;
        REQUIRE(restored.fromBinary(data));

        REQUIRE(restored.vendorId() == source.vendorId());
        REQUIRE(restored.productId() == source.productId());
        REQUIRE(restored.usbSpecVersion() == source.usbSpecVersion());
        REQUIRE(restored.deviceVersion() == source.deviceVersion());
        REQUIRE(restored.deviceClass() == source.deviceClass());
        REQUIRE(restored.subClass() == source.subClass());
        REQUIRE(restored.protocol() == source.protocol());
    }
}

// =============================================================================
// Block 2: Edge cases
// =============================================================================

TEST_CASE("JobUsbDescriptor default construction represents an empty descriptor",
          "[usb][descriptor][edge]")
{
    JobUsbDescriptor descriptor;
    REQUIRE(descriptor.vendorId() == 0);
    REQUIRE(descriptor.productId() == 0);
    REQUIRE(descriptor.usbSpecVersion() == 0);
    REQUIRE(descriptor.deviceVersion() == 0);
    REQUIRE(descriptor.deviceClass() == 0);
    REQUIRE(descriptor.subClass() == 0);
    REQUIRE(descriptor.protocol() == 0);
}

TEST_CASE("JobUsbDescriptor preserves the full width of descriptor fields",
          "[usb][descriptor][edge]")
{
    JobUsbDescriptor descriptor{
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT8_MAX,
        UINT8_MAX,
        UINT8_MAX
    };

    REQUIRE(descriptor.vendorId() == UINT16_MAX);
    REQUIRE(descriptor.productId() == UINT16_MAX);
    REQUIRE(descriptor.usbSpecVersion() == UINT16_MAX);
    REQUIRE(descriptor.deviceVersion() == UINT16_MAX);
    REQUIRE(descriptor.deviceClass() == UINT8_MAX);
    REQUIRE(descriptor.subClass() == UINT8_MAX);
    REQUIRE(descriptor.protocol() == UINT8_MAX);
}

TEST_CASE("JobUsbDescriptor copies and moves descriptor state",
          "[usb][descriptor][edge][lifetime]")
{
    JobUsbDescriptor source{
        0x1234,
        0x5678,
        0x0320,
        0x0201,
        0xEF,
        0x02,
        0x01
    };

    JobUsbDescriptor copied{source};
    REQUIRE(copied.vendorId() == source.vendorId());
    REQUIRE(copied.productId() == source.productId());
    REQUIRE(copied.usbSpecVersion() == source.usbSpecVersion());
    REQUIRE(copied.deviceVersion() == source.deviceVersion());
    REQUIRE(copied.deviceClass() == source.deviceClass());
    REQUIRE(copied.subClass() == source.subClass());
    REQUIRE(copied.protocol() == source.protocol());

    JobUsbDescriptor moved{std::move(copied)};
    REQUIRE(moved.vendorId() == 0x1234);
    REQUIRE(moved.productId() == 0x5678);
    REQUIRE(moved.usbSpecVersion() == 0x0320);
    REQUIRE(moved.deviceVersion() == 0x0201);
    REQUIRE(moved.deviceClass() == 0xEF);
    REQUIRE(moved.subClass() == 0x02);
    REQUIRE(moved.protocol() == 0x01);
}


} // namespace job::usb::tests
