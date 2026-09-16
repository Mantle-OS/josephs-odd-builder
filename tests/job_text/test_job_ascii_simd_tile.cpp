#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include <job_ascii_simd_tile.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

TEST_CASE("AsciiSimdTile exposes expected tile geometry", "[core][text][ascii][simd][tile]")
{
    STATIC_REQUIRE(AsciiSimdTile<1>::kRegisterCount == 1);
    STATIC_REQUIRE(AsciiSimdTile<2>::kRegisterCount == 2);
    STATIC_REQUIRE(AsciiSimdTile<4>::kRegisterCount == 4);
    STATIC_REQUIRE(AsciiSimdTile<1>::kByteCount == AsciiSimdTile<1>::kRegisterBytes);
    STATIC_REQUIRE(AsciiSimdTile<2>::kByteCount == AsciiSimdTile<2>::kRegisterBytes * 2);
    STATIC_REQUIRE(AsciiSimdTile<4>::kByteCount == AsciiSimdTile<4>::kRegisterBytes * 4);
    STATIC_REQUIRE(AsciiSimdTile<1>::kRegisterBytes == AsciiSimdTile<2>::kRegisterBytes);
    STATIC_REQUIRE(AsciiSimdTile<2>::kRegisterBytes == AsciiSimdTile<4>::kRegisterBytes);
}

TEST_CASE("AsciiSimdTile consumes complete ASCII tiles", "[core][text][ascii][simd][tile]")
{
    {
        auto input = makeAsciiUtf8(AsciiSimdTile<1>::kByteCount);

        const AsciiSimdTile<1>::Input tile{
            input.data(),
            AsciiSimdTile<1>::kByteCount
        };

        REQUIRE(AsciiSimdTile<1>::asciiPrefixBytes(tile) == AsciiSimdTile<1>::kByteCount);
    }

    {
        auto input = makeAsciiUtf8(AsciiSimdTile<2>::kByteCount);

        const AsciiSimdTile<2>::Input tile{
            input.data(),
            AsciiSimdTile<2>::kByteCount
        };

        REQUIRE(AsciiSimdTile<2>::asciiPrefixBytes(tile) == AsciiSimdTile<2>::kByteCount);
    }

    {
        auto input = makeAsciiUtf8(AsciiSimdTile<4>::kByteCount);

        const AsciiSimdTile<4>::Input tile{
            input.data(),
            AsciiSimdTile<4>::kByteCount
        };

        REQUIRE(AsciiSimdTile<4>::asciiPrefixBytes(tile) == AsciiSimdTile<4>::kByteCount);
    }
}

TEST_CASE("AsciiSimdTile one-register tile rejects a high-bit register", "[core][text][ascii][simd][tile]")
{
    constexpr std::size_t size = AsciiSimdTile<1>::kByteCount;

    for (std::size_t offset = 0; offset < size; ++offset) {
        auto input = makeUtf8WithHighBit(size, offset);

        const AsciiSimdTile<1>::Input tile{
            input.data(),
            AsciiSimdTile<1>::kByteCount
        };

        CAPTURE(offset);

        REQUIRE(AsciiSimdTile<1>::asciiPrefixBytes(tile) == 0);
    }
}

TEST_CASE("AsciiSimdTile two-register tile reports proven ASCII registers", "[core][text][ascii][simd][tile]")
{
    constexpr std::size_t registerBytes = AsciiSimdTile<2>::kRegisterBytes;
    constexpr std::size_t size = AsciiSimdTile<2>::kByteCount;

    for (std::size_t offset = 0; offset < size; ++offset) {
        auto input = makeUtf8WithHighBit(size, offset);

        const AsciiSimdTile<2>::Input tile{
            input.data(),
            AsciiSimdTile<2>::kByteCount
        };

        const std::size_t expected =
            offset < registerBytes ? 0 : registerBytes;

        CAPTURE(offset);
        CAPTURE(expected);

        REQUIRE(AsciiSimdTile<2>::asciiPrefixBytes(tile) == expected);
    }
}

TEST_CASE("AsciiSimdTile four-register tile reports proven ASCII registers", "[core][text][ascii][simd][tile]")
{
    constexpr std::size_t registerBytes = AsciiSimdTile<4>::kRegisterBytes;
    constexpr std::size_t size = AsciiSimdTile<4>::kByteCount;

    for (std::size_t offset = 0; offset < size; ++offset) {
        auto input = makeUtf8WithHighBit(size, offset);

        const AsciiSimdTile<4>::Input tile{
            input.data(),
            AsciiSimdTile<4>::kByteCount
        };

        const std::size_t expected =
            (offset / registerBytes) * registerBytes;

        CAPTURE(offset);
        CAPTURE(expected);

        REQUIRE(AsciiSimdTile<4>::asciiPrefixBytes(tile) == expected);
    }
}

TEST_CASE("AsciiSimdTile reports the first register containing a high-bit byte", "[core][text][ascii][simd][tile]")
{
    using Tile = AsciiSimdTile<4>;

    constexpr std::array<std::size_t, 8> offsets{
        0,
        Tile::kRegisterBytes - 1,
        Tile::kRegisterBytes,
        Tile::kRegisterBytes * 2 - 1,
        Tile::kRegisterBytes * 2,
        Tile::kRegisterBytes * 3 - 1,
        Tile::kRegisterBytes * 3,
        Tile::kByteCount - 1
    };

    for (const std::size_t offset : offsets) {
        auto input = makeUtf8WithHighBit(Tile::kByteCount, offset);

        const Tile::Input tile{
            input.data(),
            Tile::kByteCount
        };

        const std::size_t expected =
            (offset / Tile::kRegisterBytes) * Tile::kRegisterBytes;

        CAPTURE(offset);
        CAPTURE(expected);

        REQUIRE(Tile::asciiPrefixBytes(tile) == expected);
    }
}

TEST_CASE("AsciiSimdTile stops at the earliest register containing any high-bit byte", "[core][text][ascii][simd][tile]")
{
    using Tile = AsciiSimdTile<4>;

    auto input = makeAsciiUtf8(Tile::kByteCount);

    input[Tile::kRegisterBytes + 7] = 0x80;
    input[Tile::kRegisterBytes * 3 + 5] = 0xFF;

    const Tile::Input tile{
        input.data(),
        Tile::kByteCount
    };

    REQUIRE(Tile::asciiPrefixBytes(tile) == Tile::kRegisterBytes);
}

} // namespace job::core::text::tests