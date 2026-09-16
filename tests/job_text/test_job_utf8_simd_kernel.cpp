#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <job_utf8_kernel.h>
#include <job_utf8_simd_kernel.h>
#include <job_utf8_simd_tile.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

static void requireSameValidation(std::span<const std::uint8_t> input)
{
    const auto expected = Utf8Kernel::validate(input);
    const auto actual = Utf8SimdKernel::validate(input);
    REQUIRE(actual.status == expected.status);
    REQUIRE(actual.inputConsumed == expected.inputConsumed);
    REQUIRE(actual.outputProduced == expected.outputProduced);
}

// -----------------------------------------------------------------------------
// Block one: shape / codec behavior
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel exposes UTF-8 codec types", "[core][text][utf8][simd][kernel]")
{
    STATIC_REQUIRE(std::same_as<Utf8SimdKernel::Result, Utf8Kernel::Result>);
    STATIC_REQUIRE(std::same_as<Utf8SimdKernel::UnitInput, Utf8Kernel::UnitInput>);
    STATIC_REQUIRE(std::same_as<Utf8SimdKernel::UnitOutput, Utf8Kernel::UnitOutput>);
    STATIC_REQUIRE(std::same_as<Utf8SimdKernel::ScalarInput, Utf8Kernel::ScalarInput>);
    STATIC_REQUIRE(std::same_as<Utf8SimdKernel::ScalarOutput, Utf8Kernel::ScalarOutput>);

    STATIC_REQUIRE(Utf8SimdKernel::kMinimumInputBytes == Utf8SimdTile<1>::kByteCount);
}

TEST_CASE("Utf8SimdKernel delegates decode and encode semantics",
          "[core][text][utf8][simd][kernel]")
{
    std::array<UnicodeScalar, 1> scalarOutput{};

    const auto simdDecode = Utf8SimdKernel::decode(utf8Valid4, scalarOutput);
    const auto kernelDecode = Utf8Kernel::decode(utf8Valid4, scalarOutput);

    REQUIRE(simdDecode.status == kernelDecode.status);
    REQUIRE(simdDecode.inputConsumed == kernelDecode.inputConsumed);
    REQUIRE(simdDecode.outputProduced == kernelDecode.outputProduced);

    const auto scalar = UnicodeScalar::from(0x1F600);
    REQUIRE(scalar.has_value());

    const std::array<UnicodeScalar, 1> scalarInput{*scalar};
    std::array<std::uint8_t, 4> simdOutput{};
    std::array<std::uint8_t, 4> kernelOutput{};

    const auto simdEncode = Utf8SimdKernel::encode(scalarInput, simdOutput);
    const auto kernelEncode = Utf8Kernel::encode(scalarInput, kernelOutput);

    REQUIRE(simdEncode.status == kernelEncode.status);
    REQUIRE(simdEncode.inputConsumed == kernelEncode.inputConsumed);
    REQUIRE(simdEncode.outputProduced == kernelEncode.outputProduced);
    REQUIRE(simdOutput == kernelOutput);
}

// -----------------------------------------------------------------------------
// Block two: ASCII decomposition
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel validates ASCII boundary sizes identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t size : utf8SimdBoundarySizes) {
        const auto input = makeAsciiUtf8(size);

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel validates SIMD decomposition boundaries identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 19> sizes{
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
        159,
        160,
        191,
        192,
        193,
        255,
        256
    };

    for (const std::size_t size : sizes) {
        const auto input = makeAsciiUtf8(size);

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel validates 255 and 256 byte ASCII paths",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t size : std::array<std::size_t, 2>{255, 256}) {
        const auto input = makeAsciiUtf8(size);
        const auto result = Utf8SimdKernel::validate(input);

        CAPTURE(size);

        REQUIRE(result.status == Utf8Status::Complete);
        REQUIRE(result.inputConsumed == size);
        REQUIRE(result.outputProduced == 0);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block three: valid UTF-8 around SIMD boundaries
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel validates two-byte scalars across SIMD boundaries",
          "[core][text][utf8][simd][kernel]")
{
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
        writeUtf8Bytes(input, offset, utf8Valid2);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel validates three-byte scalars across SIMD boundaries",
          "[core][text][utf8][simd][kernel]")
{
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
        writeUtf8Bytes(input, offset, utf8Valid3);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel validates four-byte scalars across SIMD boundaries",
          "[core][text][utf8][simd][kernel]")
{
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
        writeUtf8Bytes(input, offset, utf8Valid4);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block four: constrained first continuations
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel enforces E0 first-continuation range identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        if (offset + utf8E0Valid.size() > 256)
            continue;

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8E0Valid);

            CAPTURE(offset);

            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8E0Invalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel enforces ED first-continuation range identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        if (offset + utf8EdValid.size() > 256)
            continue;

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8EdValid);

            CAPTURE(offset);

            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8EdInvalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel enforces F0 first-continuation range identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        if (offset + utf8F0Valid.size() > 256)
            continue;

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8F0Valid);
            CAPTURE(offset);
            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8F0Invalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel enforces F4 first-continuation range identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        if (offset + utf8F4Valid.size() > 256)
            continue;

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8F4Valid);

            CAPTURE(offset);

            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(256);
            writeUtf8Bytes(input, offset, utf8F4Invalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

// -----------------------------------------------------------------------------
// Block five: malformed UTF-8 around SIMD boundaries
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel reports lone continuation bytes identically",
          "[core][text][utf8][simd][kernel]")
{
    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        auto input = makeAsciiUtf8(256);
        input[offset] = 0x80;

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel reports invalid lead bytes identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 4> invalidLeads{
        0xC0,
        0xC1,
        0xF5,
        0xFF
    };

    for (const std::uint8_t invalid : invalidLeads) {
        for (const std::size_t offset : utf8SimdBoundaryOffsets) {
            auto input = makeAsciiUtf8(256);
            input[offset] = invalid;

            CAPTURE(invalid);
            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel reports malformed continuation sequences identically",
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
        writeUtf8Bytes(input, offset, malformed);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel reports unexpected ASCII inside multibyte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::uint8_t, 4> malformed4{
        0xF0,
        0x9F,
        0x41,
        0x80
    };

    for (const std::size_t offset : utf8SimdBoundaryOffsets) {
        if (offset + malformed4.size() > 256)
            continue;

        auto input = makeAsciiUtf8(256);
        writeUtf8Bytes(input, offset, malformed4);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block six: incomplete tails
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel reports truncated two-byte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 8> sizes{
        31,
        32,
        33,
        63,
        64,
        65,
        128,
        255
    };

    for (const std::size_t size : sizes) {
        auto input = makeAsciiUtf8(size);
        input.back() = 0xC2;

        CAPTURE(size);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel reports truncated three-byte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 8> sizes{
        31,
        32,
        33,
        63,
        64,
        65,
        128,
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

TEST_CASE("Utf8SimdKernel reports truncated four-byte sequences identically",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::array<std::size_t, 8> sizes{
        31,
        32,
        33,
        63,
        64,
        65,
        128,
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
// Block seven: every-offset valid differential stress
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for two-byte scalars at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8Valid2.size() <= size; ++offset) {
        auto input = makeAsciiUtf8(size);
        writeUtf8Bytes(input, offset, utf8Valid2);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for three-byte scalars at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8Valid3.size() <= size; ++offset) {
        auto input = makeAsciiUtf8(size);
        writeUtf8Bytes(input, offset, utf8Valid3);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for four-byte scalars at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8Valid4.size() <= size; ++offset) {
        auto input = makeAsciiUtf8(size);
        writeUtf8Bytes(input, offset, utf8Valid4);

        CAPTURE(offset);

        requireSameValidation(input);
    }
}

// -----------------------------------------------------------------------------
// Block eight: every-offset malformed differential stress
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for lone continuation bytes at every offset",
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

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for invalid lead bytes at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    constexpr std::array<std::uint8_t, 4> invalidLeads{
        0xC0,
        0xC1,
        0xF5,
        0xFF
    };

    for (const std::uint8_t invalid : invalidLeads) {
        for (std::size_t offset = 0; offset < size; ++offset) {
            auto input = makeAsciiUtf8(size);
            input[offset] = invalid;

            CAPTURE(invalid);
            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for E0 range rules at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8E0Valid.size() <= size; ++offset) {
        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8E0Valid);

            CAPTURE(offset);

            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8E0Invalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for ED range rules at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8EdValid.size() <= size; ++offset) {
        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8EdValid);

            CAPTURE(offset);

            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8EdInvalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for F0 range rules at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8F0Valid.size() <= size; ++offset) {
        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8F0Valid);

            CAPTURE(offset);

            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8F0Invalid);

            CAPTURE(offset);

            requireSameValidation(input);
        }
    }
}

TEST_CASE("Utf8SimdKernel matches Utf8Kernel for F4 range rules at every offset",
          "[core][text][utf8][simd][kernel]")
{
    constexpr std::size_t size = 512;

    for (std::size_t offset = 0; offset + utf8F4Valid.size() <= size; ++offset) {
        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8F4Valid);
            CAPTURE(offset);
            requireSameValidation(input);
        }

        {
            auto input = makeAsciiUtf8(size);
            writeUtf8Bytes(input, offset, utf8F4Invalid);
            CAPTURE(offset);
            requireSameValidation(input);
        }
    }
}

} // namespace job::core::text::tests