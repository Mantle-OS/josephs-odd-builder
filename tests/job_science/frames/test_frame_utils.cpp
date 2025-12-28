#include <catch2/catch_template_test_macros.hpp>

#include <crc32.h>
#include <endian_utils.h>

using namespace job::core;

TEST_CASE("Endian utilities: toLE32 and fromLE32", "[frames][endian]") {
    uint32_t host_value    = 0x12345678;
    uint32_t swapped_value = 0x78563412;
    if constexpr (std::endian::native == std::endian::little) {
        SECTION("Host-to-LE (toLE32) on Little-Endian") {
            REQUIRE(toLE32(host_value) == host_value);
        }

        SECTION("LE-to-Host (fromLE32) on Little-Endian") {
            REQUIRE(fromLE32(host_value) == host_value);
        }

    } else if constexpr (std::endian::native == std::endian::big) {
         SECTION("Host-to-LE (toLE32) on Big-Endian") {
             REQUIRE(toLE32(host_value) == swapped_value);
        }

        SECTION("LE-to-Host (fromLE32) on Big-Endian") {
            REQUIRE(fromLE32(swapped_value) == host_value);
        }
    }
    SECTION("Round-trip (Host -> LE -> Host)") {
        uint32_t le_value = toLE32(host_value);
        uint32_t final_host_value = fromLE32(le_value);
        REQUIRE(final_host_value == host_value);
    }
}

TEST_CASE("CRC32 computation", "[frames][crc32]") {
    SECTION("Test CRC32 computation with known data: Hello") {
        uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
        uint32_t expectedCRC = 0xF7D18982u;
        uint32_t computedCRC = Crc32::compute(data, sizeof(data));
        REQUIRE(computedCRC == expectedCRC);
    }

    SECTION("Test CRC32 with empty data") {
        std::vector<uint8_t> data;
        uint32_t expectedCRC = 0x00000000u;
        uint32_t computedCRC = Crc32::compute(data.data(), data.size());
        REQUIRE(computedCRC == expectedCRC);
    }

    SECTION("Test CRC32 with a single byte") {
        uint8_t data[] = {0x01};  // Whhat is this ?
        // known CRC32 {0x01}
        uint32_t expectedCRC = 0xA505DF1Bu;
        uint32_t computedCRC = Crc32::compute(data, sizeof(data));
        REQUIRE(computedCRC == expectedCRC);
    }

    SECTION("Test CRC32 with '123456789'") {
        const char* str = "123456789";
        uint32_t expectedCRC = 0xCBF43926u;
        uint32_t computedCRC = Crc32::compute(reinterpret_cast<const uint8_t*>(str), 9);
        REQUIRE(computedCRC == expectedCRC);
    }
}
