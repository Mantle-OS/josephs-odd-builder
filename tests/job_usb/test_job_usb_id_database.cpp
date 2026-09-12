#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <chrono>
#include <iostream>

#include <job_tmp_file.h>
#include <job_usb_id_database.h>

#include "test_job_usb_utils.h"

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: State presentation
//////////////////////////////////////////////////////////

TEST_CASE("UsbIdLoadState converts to names", "[job_usb][id][database][state][to_string]")
{
    REQUIRE(toString(UsbIdLoadState::Uninitialized) == "Uninitialized");
    REQUIRE(toString(UsbIdLoadState::Io) == "Io");
    REQUIRE(toString(UsbIdLoadState::Parsing) == "Parsing");
    REQUIRE(toString(UsbIdLoadState::UnsupportedSection) == "Unsupported Section");
    REQUIRE(toString(UsbIdLoadState::Finished) == "Finished");
    REQUIRE(toString(UsbIdLoadState::Error) == "Error");
}

//////////////////////////////////////////////////////////
// Block 2: Database load / lookup
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbIdDatabase loads and finalizes usb.ids with mmap", "[job_usb][id][database][mmap]")
{
    constexpr std::string_view source =
        "# Synthetic usb.ids database\n"
        "\n"
        "046d  Logitech, Inc.\n"
        "\tc534  Unifying Receiver\n"
        "\t\t0001  Receiver Interface\n"
        "1532  Razer USA, Ltd\n"
        "\t024e  Razer BlackWidow V3\n"
        "\n"
        "C 03  Human Interface Device\n"
        "\t01  Boot Interface Subclass\n"
        "\t\t01  Keyboard\n"
        "\t\t02  Mouse\n"
        "C 09  Hub\n"
        "\t00  Full speed hub\n"
        "\t\t00  Full speed hub\n";

    const auto path = std::filesystem::temp_directory_path() / "job_usb_id_database_mmap.ids";

    io::JobTmpFile file{path, source};

    auto &database = JobUsbIdDatabase::instance();
    database.clear();
    REQUIRE(database.state() == UsbIdLoadState::Uninitialized);
    REQUIRE_FALSE(database.ready());
    REQUIRE(database.lastError().empty());

    REQUIRE(database.load(path, true));

    REQUIRE(database.state() == UsbIdLoadState::Finished);
    REQUIRE(database.ready());
    REQUIRE(database.lastError().empty());

    //////////////////////////////////////////////////////////
    // Vendor lookup
    //////////////////////////////////////////////////////////

    REQUIRE(database.vendorName(0x046d) == "Logitech, Inc.");
    REQUIRE(database.vendorName(0x1532) == "Razer USA, Ltd");

    //////////////////////////////////////////////////////////
    // Product lookup
    //////////////////////////////////////////////////////////

    REQUIRE(database.productName(0x046d, 0xc534) == "Unifying Receiver");
    REQUIRE(database.productName(0x1532, 0x024e) == "Razer BlackWidow V3");

    //////////////////////////////////////////////////////////
    // Class hierarchy lookup
    //////////////////////////////////////////////////////////

    REQUIRE(database.className(0x03) == "Human Interface Device");
    REQUIRE(database.subclassName(0x03, 0x01) == "Boot Interface Subclass");
    REQUIRE(database.protocolName(0x03, 0x01, 0x01) == "Keyboard");
    REQUIRE(database.protocolName(0x03, 0x01, 0x02) == "Mouse");

    REQUIRE(database.className(0x09) == "Hub");
    REQUIRE(database.subclassName(0x09, 0x00) == "Full speed hub");
    REQUIRE(database.protocolName(0x09, 0x00, 0x00) == "Full speed hub");

    //////////////////////////////////////////////////////////
    // Ordinary lookup misses
    //////////////////////////////////////////////////////////

    REQUIRE(database.vendorName(0xffff).empty());
    REQUIRE(database.productName(0x046d, 0xffff).empty());

    REQUIRE(database.className(0xff).empty());
    REQUIRE(database.subclassName(0x03, 0xff).empty());
    REQUIRE(database.protocolName(0x03, 0x01, 0xff).empty());

    // Lookup misses are normal and do not poison the database.
    REQUIRE(database.state() == UsbIdLoadState::Finished);
    REQUIRE(database.ready());
    REQUIRE(database.lastError().empty());

    //////////////////////////////////////////////////////////
    // Singleton / finalization semantics
    //////////////////////////////////////////////////////////

    auto &sameDatabase = JobUsbIdDatabase::instance();

    REQUIRE(&sameDatabase == &database);

    // Finished is terminal. Another load request is harmless and does not
    // rebuild or mutate the finalized database.
    REQUIRE(database.load("/this/path/does/not/exist/usb.ids", true));

    REQUIRE(database.state() == UsbIdLoadState::Finished);
    REQUIRE(database.ready());

    REQUIRE(database.vendorName(0x046d) == "Logitech, Inc.");
    REQUIRE(database.productName(0x046d, 0xc534) == "Unifying Receiver");

#ifdef JOB_TEST_BENCHMARKS

    BENCHMARK("JobUsbIdDatabase vendor lookup") {
        return database.vendorName(0x046d);
    };

    BENCHMARK("JobUsbIdDatabase vendor lookup with compiler barrier") {
        const std::string_view value = database.vendorName(0x046d);

        asm volatile("" : : "r"(value.data()), "r"(value.size()) : "memory");

        return value;
    };

    BENCHMARK("JobUsbIdDatabase product lookup") {
        return database.productName(0x046d, 0xc534);
    };

    BENCHMARK("JobUsbIdDatabase product lookup with compiler barrier") {
        const std::string_view value = database.productName(0x046d, 0xc534);

        asm volatile("" : : "r"(value.data()), "r"(value.size()) : "memory");

        return value;
    };

    BENCHMARK("JobUsbIdDatabase protocol lookup") {
        return database.protocolName(0x03, 0x01, 0x01);
    };

    BENCHMARK("JobUsbIdDatabase protocol lookup with compiler barrier") {
        const std::string_view value = database.protocolName(0x03, 0x01, 0x01);

        asm volatile("" : : "r"(value.data()), "r"(value.size()) : "memory");

        return value;
    };




#endif
}





TEST_CASE("JobUsbIdDatabase loads and benchmarks the whole database", "[job_usb][id][database][local][benchmark]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    const std::filesystem::path path = "/usr/share/hwdata/usb.ids";

    REQUIRE(std::filesystem::exists(path));
    REQUIRE(database.load(path));
    REQUIRE(database.ready());


    for (const auto vendor : test_vendors)
        REQUIRE_FALSE(database.vendorName(vendor).empty());

    std::size_t vIdx = 0;

    BENCHMARK("Full DB: Vendor lookup (cycled targets)") {
        const auto id = test_vendors[(vIdx++) % std::size(test_vendors)];
        return database.vendorName(id);
    };

    for (const auto &[vendor, product] : test_products) {
        CAPTURE(vendor, product);
        REQUIRE_FALSE(database.productName(vendor, product).empty());
    }

    std::size_t pIdx = 0;

    BENCHMARK("Full DB: Product lookup (cycled targets)") {
        const auto &[vendor, product] = test_products[(pIdx++) % std::size(test_products)];
        return database.productName(vendor, product);
    };


    database.clear();
};

} // namespace job::usb::tests