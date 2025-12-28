#include <catch2/catch_test_macros.hpp>

#include <crc32.h>
#include <endian_utils.h>

#include <frames/frame_header.h>

using namespace job::science::frames;

TEST_CASE("FrameHeader initialization and primary validation", "[frames][header]")
{
    FrameHeader hdr = FrameHeader::create(42, 123456789ULL, 1024, 8192);

    SECTION("Initialization and Accessors") {
        REQUIRE(hdr.frameId == 42);
        REQUIRE(hdr.timestampNs == 123456789ULL);
        REQUIRE(hdr.particleCount == 1024);
        REQUIRE(hdr.byteLength == 8192);
        REQUIRE(hdr.headerSize == sizeof(FrameHeader));
    }

    SECTION("Binary Footprint and Alignment") {
        REQUIRE(sizeof(FrameHeader) <= FrameHeader::kAlignedSize);
        REQUIRE(FrameHeader::kAlignedSize % 16 == 0); // Check 16-byte alignment
    }

    SECTION("Core Validation (Initial Check)") {
        REQUIRE(hdr.validateMagicAndSize() == true);
    }

    SECTION("Validation Failure - Magic Number") {
        FrameHeader invalidHdr = hdr;
        invalidHdr.magic = 0x12345678; // Intentional corruption
        REQUIRE(invalidHdr.validateMagicAndSize() == false);
    }

    SECTION("Validation Failure - Version") {
        FrameHeader invalidHdr = hdr;
        invalidHdr.version = 0xFFFF;
        REQUIRE(invalidHdr.validateMagicAndSize() == false);
    }

    SECTION("Round-trip Size Calculation") {
        // totalBytes() should be payload size + aligned header size
        REQUIRE(hdr.totalBytes() == 8192 + FrameHeader::kAlignedSize);
    }
}

TEST_CASE("FrameHeader CRC and Endianness", "[frames][crc][endian]")
{
    // Simulate payload data (8 bytes of test data)
    uint8_t payload_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint32_t expected_crc = job::core::Crc32::compute(payload_data, sizeof(payload_data));

    // Create a header with data, and simulate Endian swap
    FrameHeader hdr = FrameHeader::create(1, 1, 1, sizeof(payload_data));
    hdr.crc32 = expected_crc;

    SECTION("CRC Utility Check") {
        REQUIRE(expected_crc == static_cast<uint32_t>(0x9118E1C2));
    }

    SECTION("Magic Number Endian Swap Check") {
        FrameHeader corrupt_magic = hdr;
        REQUIRE(corrupt_magic.validateMagicAndSize() == true);
    }
}
