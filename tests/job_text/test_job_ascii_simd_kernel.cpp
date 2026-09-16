#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <concepts>

#include <catch2/catch_test_macros.hpp>

#include <job_utf8_kernel.h>
#include <job_ascii_simd_kernel.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

static void requireSameValidation(std::span<const std::uint8_t> input)
{
    const auto expected = Utf8Kernel::validate(input);
    const auto actual = AsciiSimdKernel::validate(input);

    REQUIRE(actual.status == expected.status);
    REQUIRE(actual.inputConsumed == expected.inputConsumed);
    REQUIRE(actual.outputProduced == expected.outputProduced);
}

static void writeBytes(std::vector<std::uint8_t> &input, std::size_t offset, std::span<const std::uint8_t> bytes)
{
    REQUIRE(offset + bytes.size() <= input.size());
    for (std::size_t i = 0; i < bytes.size(); ++i)
        input[offset + i] = bytes[i];
}

// -----------------------------------------------------------------------------
// Block one: shape / codec behavior
// -----------------------------------------------------------------------------

TEST_CASE("AsciiSimdKernel exposes UTF-8 codec types", "[core][text][utf8][simd][kernel]")
{
    STATIC_REQUIRE(std::same_as<AsciiSimdKernel::Result, Utf8Kernel::Result>);
    STATIC_REQUIRE(std::same_as<AsciiSimdKernel::UnitInput, Utf8Kernel::UnitInput>);
    STATIC_REQUIRE(std::same_as<AsciiSimdKernel::UnitOutput, Utf8Kernel::UnitOutput>);
    STATIC_REQUIRE(std::same_as<AsciiSimdKernel::ScalarInput, Utf8Kernel::ScalarInput>);
    STATIC_REQUIRE(std::same_as<AsciiSimdKernel::ScalarOutput, Utf8Kernel::ScalarOutput>);

    STATIC_REQUIRE(AsciiSimdKernel::kMinimumInputBytes == AsciiSimdTile<1>::kByteCount);
}

TEST_CASE("AsciiSimdKernel delegates decode and encode semantics", "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 4> encoded{
        0xF0,
        0x9F,
        0x98,
        0x80
    };

    std::array<UnicodeScalar, 1> scalarOutput{};

    const auto simdDecode = AsciiSimdKernel::decode(encoded, scalarOutput);
    const auto kernelDecode = Utf8Kernel::decode(encoded, scalarOutput);

    REQUIRE(simdDecode.status == kernelDecode.status);
    REQUIRE(simdDecode.inputConsumed == kernelDecode.inputConsumed);
    REQUIRE(simdDecode.outputProduced == kernelDecode.outputProduced);

    const auto scalar = UnicodeScalar::from(0x1F600);
    REQUIRE(scalar.has_value());

    const std::array<UnicodeScalar, 1> scalarInput{*scalar};
    std::array<std::uint8_t, 4> simdOutput{};
    std::array<std::uint8_t, 4> kernelOutput{};

    const auto simdEncode = AsciiSimdKernel::encode(scalarInput, simdOutput);
    const auto kernelEncode = Utf8Kernel::encode(scalarInput, kernelOutput);

    REQUIRE(simdEncode.status == kernelEncode.status);
    REQUIRE(simdEncode.inputConsumed == kernelEncode.inputConsumed);
    REQUIRE(simdEncode.outputProduced == kernelEncode.outputProduced);
    REQUIRE(simdOutput == kernelOutput);
}

// -----------------------------------------------------------------------------
// Block two: ASCII decomposition
// -----------------------------------------------------------------------------

TEST_CASE("AsciiSimdKernel validates ASCII boundary sizes identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t size : utf8SimdBoundarySizes) {
        const auto input = makeAsciiUtf8(size);

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel validates SIMD decomposition boundaries",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 15> sizes{
        31,
        32,
        33,
        63,
        64,
        65,
        95,
        96,
        97,
        127,
        128,
        129,
        191,
        192,
        193
    };

    for (const std::size_t size : sizes) {
        const auto input = makeAsciiUtf8(size);

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel validates 255 and 256 byte ASCII paths",
          "[core][text][utf8][simd][kernel]")
{
    {
        const auto input = makeAsciiUtf8(255);

        const auto result = AsciiSimdKernel::validate(input);

        REQUIRE(result.status == Utf8Status::Complete);
        REQUIRE(result.inputConsumed == 255);
        REQUIRE(result.outputProduced == 0);

        requireSameValidation(input);
    }

    {
        const auto input = makeAsciiUtf8(256);

        const auto result = AsciiSimdKernel::validate(input);

        REQUIRE(result.status == Utf8Status::Complete);
        REQUIRE(result.inputConsumed == 256);
        REQUIRE(result.outputProduced == 0);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block three: valid UTF-8 around SIMD boundaries
// -----------------------------------------------------------------------------

TEST_CASE("AsciiSimdKernel validates two-byte scalars across SIMD boundaries",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 2> scalar{
        0xC2,
        0xA2
    };

    constexpr std::array<std::size_t, 12> offsets{
        30,
        31,
        32,
        62,
        63,
        64,
        94,
        95,
        96,
        126,
        127,
        128
    };

    for (const std::size_t offset : offsets) {
        auto input = makeAsciiUtf8(256);

        writeBytes(input, offset, scalar);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel validates three-byte scalars across SIMD boundaries",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 3> scalar{
        0xE2,
        0x82,
        0xAC
    };

    constexpr std::array<std::size_t, 16> offsets{
        29,
        30,
        31,
        32,
        61,
        62,
        63,
        64,
        93,
        94,
        95,
        96,
        125,
        126,
        127,
        128
    };

    for (const std::size_t offset : offsets) {
        auto input = makeAsciiUtf8(256);

        writeBytes(input, offset, scalar);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel validates four-byte scalars across SIMD boundaries",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 4> scalar{
        0xF0,
        0x9F,
        0x98,
        0x80
    };

    constexpr std::array<std::size_t, 20> offsets{
        28,
        29,
        30,
        31,
        32,
        60,
        61,
        62,
        63,
        64,
        92,
        93,
        94,
        95,
        96,
        124,
        125,
        126,
        127,
        128
    };

    for (const std::size_t offset : offsets) {
        auto input = makeAsciiUtf8(256);

        writeBytes(input, offset, scalar);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block four: malformed UTF-8 around SIMD boundaries
// -----------------------------------------------------------------------------

TEST_CASE("AsciiSimdKernel reports lone continuation bytes identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        auto input = makeAsciiUtf8(256);
        input[offset] = 0x80;

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports invalid lead bytes identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        auto input = makeAsciiUtf8(256);
        input[offset] = 0xFF;

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports malformed continuation sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 3> malformed{
        0xE2,
        0x28,
        0xA1
    };

    constexpr std::array<std::size_t, 12> offsets{
        30,
        31,
        32,
        62,
        63,
        64,
        94,
        95,
        96,
        126,
        127,
        128
    };

    for (const std::size_t offset : offsets) {
        auto input = makeAsciiUtf8(256);

        writeBytes(input, offset, malformed);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports overlong encodings identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 2> overlong{
        0xC0,
        0xAF
    };

    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        auto input = makeAsciiUtf8(256);

        if (offset + overlong.size() > input.size())
            continue;

        writeBytes(input, offset, overlong);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports surrogate encodings identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 3> surrogate{
        0xED,
        0xA0,
        0x80
    };

    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        auto input = makeAsciiUtf8(256);

        if (offset + surrogate.size() > input.size())
            continue;

        writeBytes(input, offset, surrogate);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports values above Unicode range identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 4> tooLarge{
        0xF4,
        0x90,
        0x80,
        0x80
    };

    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        auto input = makeAsciiUtf8(256);

        if (offset + tooLarge.size() > input.size())
            continue;

        writeBytes(input, offset, tooLarge);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block five: incomplete tails
// -----------------------------------------------------------------------------

TEST_CASE("AsciiSimdKernel reports truncated two-byte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 5> sizes{
        31,
        32,
        63,
        64,
        255
    };

    for (const std::size_t size : sizes) {
        auto input = makeAsciiUtf8(size);
        input.back() = 0xC2;

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports truncated three-byte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 5> sizes{
        31,
        32,
        63,
        64,
        255
    };

    for (const std::size_t size : sizes) {
        auto input = makeAsciiUtf8(size);

        input[size - 2] = 0xE2;
        input[size - 1] = 0x82;

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel reports truncated four-byte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 5> sizes{
        31,
        32,
        63,
        64,
        255
    };

    for (const std::size_t size : sizes) {
        auto input = makeAsciiUtf8(size);

        input[size - 3] = 0xF0;
        input[size - 2] = 0x9F;
        input[size - 1] = 0x98;

        CAPTURE(size);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block six: every-offset differential stress
// -----------------------------------------------------------------------------

TEST_CASE("AsciiSimdKernel matches Utf8Kernel for a two-byte scalar at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 2> scalar{
        0xC2,
        0xA2
    };

    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + scalar.size() <= size; ++offset) {
        auto input = makeAsciiUtf8(size);

        writeBytes(input, offset, scalar);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel matches Utf8Kernel for a four-byte scalar at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 4> scalar{
        0xF0,
        0x9F,
        0x98,
        0x80
    };

    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + scalar.size() <= size; ++offset) {
        auto input = makeAsciiUtf8(size);

        writeBytes(input, offset, scalar);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("AsciiSimdKernel matches Utf8Kernel for malformed input at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset < size; ++offset) {
        auto input = makeAsciiUtf8(size);
        input[offset] = 0x80;

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

} // namespace job::core::text::tests