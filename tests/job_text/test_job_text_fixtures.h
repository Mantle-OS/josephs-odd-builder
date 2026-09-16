#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>


#include <job_utf8_status.h>

namespace job::core::text::tests {

// Smells Like API..
template <typename Tile, std::size_t N>
static void writeBytes(std::array<std::uint8_t, Tile::kByteCount> &input, std::size_t offset, const std::array<std::uint8_t, N> &bytes)
{
    REQUIRE(offset + bytes.size() <= input.size());

    for (std::size_t i = 0; i < bytes.size(); ++i)
        input[offset + i] = bytes[i];
}
// Smells API Like..
template <typename Container, std::size_t N>
inline void writeUtf8Bytes(Container &input, std::size_t offset, const std::array<std::uint8_t, N> &bytes)
{
    REQUIRE(offset + bytes.size() <= input.size());
    for (std::size_t i = 0; i < bytes.size(); ++i)
        input[offset + i] = bytes[i];
}

// Smells Like API Like..
inline void writeUtf8Bytes(std::vector<std::uint8_t> &input, std::size_t offset, std::span<const std::uint8_t> bytes)
{
    for (std::size_t i = 0; i < bytes.size(); ++i)
        input[offset + i] = bytes[i];
}

// Smells API Like..
template <typename Tile>
[[nodiscard]] static std::array<std::uint8_t, Tile::kByteCount> makeAsciiTile()
{
    std::array<std::uint8_t, Tile::kByteCount> input{};
    input.fill(static_cast<std::uint8_t>('A'));
    return input;
}

// Smells API Like..
template <typename Tile>
[[nodiscard]] static std::array<char16_t, Tile::kUnitCount> makeBmpTile()
{
    std::array<char16_t, Tile::kUnitCount> input{};
    input.fill(u'A');
    return input;
}

// Smells API Like.. with pre(offset + 1 < input.size()) contract then job_text knows these calls.
template <typename Tile>
static void writeUtf16Pair(std::array<char16_t, Tile::kUnitCount> &input,
                           std::size_t offset,
                           char16_t high,
                           char16_t low)
{
    REQUIRE(offset + 1 < input.size());
    input[offset]       = high;
    input[offset + 1]   = low;
}

[[nodiscard]] inline std::vector<std::uint8_t> makeAsciiUtf8(std::size_t size)
{
    return std::vector<std::uint8_t>(size, static_cast<std::uint8_t>('A'));
}

[[nodiscard]] inline std::vector<std::uint8_t> makeUtf8WithHighBit(std::size_t size, std::size_t offset)
{
    auto input = makeAsciiUtf8(size);
    input[offset] = 0x80;
    return input;
}


// Good fixtures below
struct Utf8BoundaryFixture
{
    std::array<std::uint8_t, 4> units{};
    std::size_t                 size{};
    char32_t                    expected{};
};

struct Utf16ScalarBoundaryFixture
{
    std::array<char16_t, 2> units{};
    std::size_t             size{};
    char32_t                expected{};
};

struct Utf16BoundaryFixture
{
    char32_t scalar{};
    char16_t high{};
    char16_t low{};
};

struct Utf32BoundaryFixture
{
    char32_t unit{};
    char32_t expected{};
};

struct TextTranscodeScalarFixture
{
    char32_t scalar{};
};


//-START SIMD TESTS
struct Utf8SizeFixture
{
    std::size_t size{};
};

struct Utf8OffsetFixture
{
    std::size_t size{};
    std::size_t offset{};
};

struct Utf8ValidationFixture
{
    std::vector<std::uint8_t>   input;
    Utf8Status                  status{};
    std::size_t                 inputConsumed{};
};


// START TEST DATA
inline constexpr std::array<std::size_t, 14> utf8SimdBoundaryOffsets{ 0, 1, 30, 31, 32, 33, 62, 63, 64, 95, 96, 126, 127, 128 };
inline constexpr std::array<std::size_t, 16> utf8SimdBoundarySizes{   0, 1, 15, 31, 32, 33, 63, 64, 65, 95, 96, 127, 128, 129, 255, 256};

// UTF-8 SIMD validation fixtures
inline constexpr std::array<std::uint8_t, 2> utf8Valid2{ 0xC2, 0xA2 };
inline constexpr std::array<std::uint8_t, 3> utf8Valid3{ 0xE2, 0x82, 0xAC };
inline constexpr std::array<std::uint8_t, 4> utf8Valid4{ 0xF0, 0x9F, 0x98, 0x80 };

// E0 requires A0..BF.
inline constexpr std::array<std::uint8_t, 3> utf8E0Valid{   0xE0, 0xA0, 0x80 };
inline constexpr std::array<std::uint8_t, 3> utf8E0Invalid{ 0xE0, 0x9F, 0x80 };

// ED requires 80..9F.
inline constexpr std::array<std::uint8_t, 3> utf8EdValid{   0xED, 0x9F, 0x80 };
inline constexpr std::array<std::uint8_t, 3> utf8EdInvalid{ 0xED, 0xA0, 0x80 };

// F0 requires 90..BF.
inline constexpr std::array<std::uint8_t, 4> utf8F0Valid{   0xF0, 0x90, 0x80, 0x80 };
inline constexpr std::array<std::uint8_t, 4> utf8F0Invalid{ 0xF0, 0x8F, 0x80, 0x80 };

// F4 requires 80..8F.
inline constexpr std::array<std::uint8_t, 4> utf8F4Valid{   0xF4, 0x8F, 0x80, 0x80 };
inline constexpr std::array<std::uint8_t, 4> utf8F4Invalid{ 0xF4, 0x90, 0x80, 0x80 };

// UTF-16 SIMD validation
inline constexpr char16_t utf16HighMin = 0xD800;
inline constexpr char16_t utf16HighMax = 0xDBFF;
inline constexpr char16_t utf16LowMin  = 0xDC00;
inline constexpr char16_t utf16LowMax  = 0xDFFF;

inline constexpr Utf16BoundaryFixture utf16PairMin{
    .scalar = 0x10000,
    .high   = 0xD800,
    .low    = 0xDC00
};

inline constexpr Utf16BoundaryFixture utf16PairMax{
    .scalar = 0x10FFFF,
    .high   = 0xDBFF,
    .low    = 0xDFFF
};

// UTF-32 SIMD validation
inline constexpr char32_t utf32ScalarMin        = 0x00000000;
inline constexpr char32_t utf32SurrogateBefore  = 0x0000D7FF;
inline constexpr char32_t utf32SurrogateMin     = 0x0000D800;
inline constexpr char32_t utf32SurrogateMax     = 0x0000DFFF;
inline constexpr char32_t utf32SurrogateAfter   = 0x0000E000;
inline constexpr char32_t utf32ScalarMax        = 0x0010FFFF;
inline constexpr char32_t utf32AboveScalarMax   = 0x00110000;

} // namespace job::core::text::tests