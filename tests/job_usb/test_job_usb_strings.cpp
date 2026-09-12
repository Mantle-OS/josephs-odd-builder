#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

#include <job_usb_strings.h>

namespace job::usb::tests {

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobUsbStrings describes optional USB string descriptors",
          "[usb][strings][usage]")
{
    JobUsbStrings strings{
        "JOB Inc.",
        "SOM-215GS",
        "ABC123456"
    };

    REQUIRE(strings.manufacturer());
    REQUIRE(strings.product());
    REQUIRE(strings.serialNumber());

    REQUIRE(strings.manufacturerView() == "JOB Inc.");
    REQUIRE(strings.productView() == "SOM-215GS");
    REQUIRE(strings.serialNumberView() == "ABC123456");
}

TEST_CASE("JobUsbStrings fields can be updated after construction",
          "[usb][strings][usage]")
{
    JobUsbStrings strings;

    strings.setManufacturer("Joseph's Odd Builder");
    strings.setProduct("USB Test Device");
    strings.setSerialNumber("JOB-0001");

    REQUIRE(strings.manufacturer());
    REQUIRE(strings.product());
    REQUIRE(strings.serialNumber());

    REQUIRE(strings.manufacturerView() == "Joseph's Odd Builder");
    REQUIRE(strings.productView() == "USB Test Device");
    REQUIRE(strings.serialNumberView() == "JOB-0001");
}

TEST_CASE("JobUsbStrings factories construct populated string descriptors",
          "[usb][strings][factory]")
{
    auto shared = JobUsbStrings::createShared(
        "JOB Inc.",
        "SOM-A5D36",
        "SHARED-001"
        );

    REQUIRE(shared);
    REQUIRE(shared->manufacturerView() == "JOB Inc.");
    REQUIRE(shared->productView() == "SOM-A5D36");
    REQUIRE(shared->serialNumberView() == "SHARED-001");

    auto unique = JobUsbStrings::createUniq(
        "JOB",
        "USB Fixture",
        "UNIQUE-001"
        );

    REQUIRE(unique);
    REQUIRE(unique->manufacturerView() == "JOB");
    REQUIRE(unique->productView() == "USB Fixture");
    REQUIRE(unique->serialNumberView() == "UNIQUE-001");
}

TEST_CASE("JobUsbStrings survives JOB serialization round trips",
          "[usb][strings][serialization]")
{
    JobUsbStrings source{
        "JOB Inc.",
        "SOM-35D1F",
        "35D1F-TEST-001"
    };

    SECTION("JSON")
    {
        std::string data;
        REQUIRE(source.toJobJson(data));

        JobUsbStrings restored;
        REQUIRE(restored.fromJobJson(data));

        REQUIRE(restored.manufacturer() == source.manufacturer());
        REQUIRE(restored.product() == source.product());
        REQUIRE(restored.serialNumber() == source.serialNumber());
    }

    SECTION("YAML")
    {
        std::string data;
        REQUIRE(source.toJobYaml(data));

        JobUsbStrings restored;
        REQUIRE(restored.fromJobYaml(data));

        REQUIRE(restored.manufacturer() == source.manufacturer());
        REQUIRE(restored.product() == source.product());
        REQUIRE(restored.serialNumber() == source.serialNumber());
    }

    SECTION("binary")
    {
        std::vector<std::uint8_t> data;
        REQUIRE(source.toBinary(data));

        JobUsbStrings restored;
        REQUIRE(restored.fromBinary(data));

        REQUIRE(restored.manufacturer() == source.manufacturer());
        REQUIRE(restored.product() == source.product());
        REQUIRE(restored.serialNumber() == source.serialNumber());
    }
}

// =============================================================================
// Block 2: Edge cases
// =============================================================================

TEST_CASE("JobUsbStrings default construction represents unavailable descriptors",
          "[usb][strings][edge]")
{
    JobUsbStrings strings;

    REQUIRE_FALSE(strings.manufacturer());
    REQUIRE_FALSE(strings.product());
    REQUIRE_FALSE(strings.serialNumber());

    REQUIRE(strings.manufacturerView().empty());
    REQUIRE(strings.productView().empty());
    REQUIRE(strings.serialNumberView().empty());
}

TEST_CASE("JobUsbStrings distinguishes missing strings from present empty strings",
          "[usb][strings][edge]")
{
    JobUsbStrings strings;

    REQUIRE_FALSE(strings.manufacturer());
    REQUIRE(strings.manufacturerView().empty());

    strings.setManufacturer(std::string{});

    REQUIRE(strings.manufacturer());
    REQUIRE(strings.manufacturer()->empty());
    REQUIRE(strings.manufacturerView().empty());
}

TEST_CASE("JobUsbStrings supports partially populated USB descriptors",
          "[usb][strings][edge]")
{
    JobUsbStrings strings{
        "JOB Inc.",
        std::nullopt,
        "SERIAL-ONLY-WITH-MANUFACTURER"
    };

    REQUIRE(strings.manufacturer());
    REQUIRE_FALSE(strings.product());
    REQUIRE(strings.serialNumber());

    REQUIRE(strings.manufacturerView() == "JOB Inc.");
    REQUIRE(strings.productView().empty());
    REQUIRE(strings.serialNumberView() == "SERIAL-ONLY-WITH-MANUFACTURER");
}

TEST_CASE("JobUsbStrings copy preserves independent string state",
          "[usb][strings][edge][lifetime]")
{
    JobUsbStrings source{
        "Manufacturer A",
        "Product A",
        "Serial A"
    };

    JobUsbStrings copied{source};

    REQUIRE(copied.manufacturer() == source.manufacturer());
    REQUIRE(copied.product() == source.product());
    REQUIRE(copied.serialNumber() == source.serialNumber());

    copied.setManufacturer("Manufacturer B");
    copied.setProduct("Product B");
    copied.setSerialNumber("Serial B");

    REQUIRE(source.manufacturerView() == "Manufacturer A");
    REQUIRE(source.productView() == "Product A");
    REQUIRE(source.serialNumberView() == "Serial A");

    REQUIRE(copied.manufacturerView() == "Manufacturer B");
    REQUIRE(copied.productView() == "Product B");
    REQUIRE(copied.serialNumberView() == "Serial B");
}

TEST_CASE("JobUsbStrings moves complete string descriptor state",
          "[usb][strings][edge][lifetime]")
{
    JobUsbStrings source{
        "Move Manufacturer",
        "Move Product",
        "Move Serial"
    };

    JobUsbStrings moved{std::move(source)};

    REQUIRE(moved.manufacturerView() == "Move Manufacturer");
    REQUIRE(moved.productView() == "Move Product");
    REQUIRE(moved.serialNumberView() == "Move Serial");
}

} // namespace job::usb::tests