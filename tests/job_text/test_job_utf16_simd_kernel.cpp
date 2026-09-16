#include <array>
#include <cstddef>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <job_utf16_kernel.h>
#include <job_utf16_simd_kernel.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: basic behavior / scalar remainder
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel validates empty input",
          "[core][text][utf16][simd][kernel]")
{
    const std::vector<char16_t> input;

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == 0);
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates input smaller than one SIMD register",
          "[core][text][utf16][simd][kernel]")
{
    constexpr std::size_t size = Utf16SimdKernel::kMinimumInputUnits - 1;
    const std::vector<char16_t> input(size, u'A');

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates exactly one SIMD register",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile1;

    const std::vector<char16_t> input(Tile::kUnitCount, u'A');

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

// -----------------------------------------------------------------------------
// Block two: final SIMD scheduler decomposition
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel validates exact SIMD tile decompositions",
          "[core][text][utf16][simd][kernel]")
{
    using Tile1 = Utf16SimdKernel::Tile1;
    using Tile2 = Utf16SimdKernel::Tile2;
    using Tile4 = Utf16SimdKernel::Tile4;
    using Tile8 = Utf16SimdKernel::Tile8;

    constexpr std::array<std::size_t, 11> sizes{
        Tile1::kUnitCount,
        Tile2::kUnitCount,
        Tile2::kUnitCount + Tile1::kUnitCount,

        Tile4::kUnitCount,
        Tile4::kUnitCount + Tile1::kUnitCount,
        Tile4::kUnitCount + Tile2::kUnitCount,
        Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount,

        Tile8::kUnitCount,
        Tile8::kUnitCount + Tile1::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount
    };

    for (const std::size_t size : sizes) {
        const std::vector<char16_t> input(size, u'A');

        CAPTURE(size);

        const auto result = Utf16SimdKernel::validate(input);

        REQUIRE(result.status == Utf16Status::Complete);
        REQUIRE(result.inputConsumed == input.size());
        REQUIRE(result.outputProduced == 0);
    }
}

TEST_CASE("Utf16SimdKernel validates repeated Tile8 input",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::size_t tileCount = 8;
    const std::vector<char16_t> input(Tile::kUnitCount * tileCount, u'A');

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates repeated Tile8 input with scalar tail",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::array<std::size_t, 5> tails{
        1,
        2,
        3,
        Utf16SimdKernel::kMinimumInputUnits - 2,
        Utf16SimdKernel::kMinimumInputUnits - 1
    };

    for (const std::size_t tail : tails) {
        const std::size_t size = Tile::kUnitCount * 2 + tail;
        const std::vector<char16_t> input(size, u'A');

        CAPTURE(tail);
        CAPTURE(size);

        const auto result = Utf16SimdKernel::validate(input);

        REQUIRE(result.status == Utf16Status::Complete);
        REQUIRE(result.inputConsumed == input.size());
        REQUIRE(result.outputProduced == 0);
    }
}

// -----------------------------------------------------------------------------
// Block three: state propagation through scheduled SIMD tiles
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel validates surrogate pairs across internal Tile8 register boundaries",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    for (std::size_t registerIndex = 1; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::vector<char16_t> input(Tile::kUnitCount, u'A');

        const std::size_t boundary = registerIndex * Tile::kRegisterUnits;
        input[boundary - 1] = utf16HighMin;
        input[boundary] = utf16LowMin;

        CAPTURE(registerIndex);
        CAPTURE(boundary);

        const auto result = Utf16SimdKernel::validate(input);

        REQUIRE(result.status == Utf16Status::Complete);
        REQUIRE(result.inputConsumed == input.size());
        REQUIRE(result.outputProduced == 0);
    }
}

TEST_CASE("Utf16SimdKernel validates surrogate pair across consecutive Tile8 iterations",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    std::vector<char16_t> input(Tile::kUnitCount * 2, u'A');

    input[Tile::kUnitCount - 1] = utf16HighMin;
    input[Tile::kUnitCount] = utf16LowMin;

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates surrogate pair across Tile8 to Tile4 boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile8 = Utf16SimdKernel::Tile8;
    using Tile4 = Utf16SimdKernel::Tile4;

    std::vector<char16_t> input(Tile8::kUnitCount + Tile4::kUnitCount, u'A');

    input[Tile8::kUnitCount - 1] = utf16HighMin;
    input[Tile8::kUnitCount] = utf16LowMin;

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates surrogate pair across Tile4 to Tile2 boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile4 = Utf16SimdKernel::Tile4;
    using Tile2 = Utf16SimdKernel::Tile2;

    std::vector<char16_t> input(Tile4::kUnitCount + Tile2::kUnitCount, u'A');

    input[Tile4::kUnitCount - 1] = utf16HighMin;
    input[Tile4::kUnitCount] = utf16LowMin;

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates surrogate pair across Tile2 to Tile1 boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile2 = Utf16SimdKernel::Tile2;
    using Tile1 = Utf16SimdKernel::Tile1;

    std::vector<char16_t> input(Tile2::kUnitCount + Tile1::kUnitCount, u'A');

    input[Tile2::kUnitCount - 1] = utf16HighMin;
    input[Tile2::kUnitCount] = utf16LowMin;

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

// -----------------------------------------------------------------------------
// Block four: SIMD to scalar restart
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel completes surrogate pair across SIMD to scalar boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile1;

    std::vector<char16_t> input(Tile::kUnitCount + 1, u'A');

    input[Tile::kUnitCount - 1] = utf16HighMin;
    input[Tile::kUnitCount] = utf16LowMin;

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel matches scalar for truncated high surrogate at SIMD boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile1;

    std::vector<char16_t> input(Tile::kUnitCount, u'A');
    input.back() = utf16HighMin;

    const auto simdResult = Utf16SimdKernel::validate(input);
    const auto scalarResult = Utf16Kernel::validate(input);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
}

TEST_CASE("Utf16SimdKernel matches scalar for BMP after high surrogate at SIMD boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile1;

    std::vector<char16_t> input(Tile::kUnitCount + 1, u'A');

    input[Tile::kUnitCount - 1] = utf16HighMin;
    input[Tile::kUnitCount] = u'A';

    const auto simdResult = Utf16SimdKernel::validate(input);
    const auto scalarResult = Utf16Kernel::validate(input);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
}

TEST_CASE("Utf16SimdKernel matches scalar for high surrogate after high surrogate at SIMD boundary",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile1;

    std::vector<char16_t> input(Tile::kUnitCount + 1, u'A');

    input[Tile::kUnitCount - 1] = utf16HighMin;
    input[Tile::kUnitCount] = utf16HighMax;

    const auto simdResult = Utf16SimdKernel::validate(input);
    const auto scalarResult = Utf16Kernel::validate(input);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
}

// -----------------------------------------------------------------------------
// Block five: malformed SIMD fallback
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel matches scalar validation for malformed positions inside Tile8",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::array<std::size_t, 12> offsets{
        0,
        1,
        Tile::kRegisterUnits - 1,
        Tile::kRegisterUnits,
        Tile::kRegisterUnits + 1,
        Tile::kRegisterUnits * 2,
        Tile::kRegisterUnits * 3,
        Tile::kRegisterUnits * 4,
        Tile::kRegisterUnits * 6,
        Tile::kRegisterUnits * 7,
        Tile::kUnitCount - 2,
        Tile::kUnitCount - 1
    };

    for (const std::size_t offset : offsets) {
        std::vector<char16_t> input(Tile::kUnitCount, u'A');
        input[offset] = utf16LowMin;

        CAPTURE(offset);

        const auto simdResult = Utf16SimdKernel::validate(input);
        const auto scalarResult = Utf16Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

TEST_CASE("Utf16SimdKernel matches scalar validation for malformed input after complete Tile8 blocks",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::size_t prefixTiles = 3;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::vector<char16_t> input(Tile::kUnitCount * (prefixTiles + 1), u'A');

        const std::size_t offset =
            Tile::kUnitCount * prefixTiles +
            registerIndex * Tile::kRegisterUnits +
            7;

        input[offset] = utf16LowMin;

        CAPTURE(registerIndex);
        CAPTURE(offset);

        const auto simdResult = Utf16SimdKernel::validate(input);
        const auto scalarResult = Utf16Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

// -----------------------------------------------------------------------------
// Block six: scalar/SIMD equivalence across scheduler boundaries
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel matches scalar validation across scheduler boundary sizes",
          "[core][text][utf16][simd][kernel]")
{
    using Tile1 = Utf16SimdKernel::Tile1;
    using Tile2 = Utf16SimdKernel::Tile2;
    using Tile4 = Utf16SimdKernel::Tile4;
    using Tile8 = Utf16SimdKernel::Tile8;

    constexpr std::array<std::size_t, 22> sizes{
        0,
        1,

        Tile1::kUnitCount - 1,
        Tile1::kUnitCount,
        Tile1::kUnitCount + 1,

        Tile2::kUnitCount - 1,
        Tile2::kUnitCount,
        Tile2::kUnitCount + 1,

        Tile4::kUnitCount - 1,
        Tile4::kUnitCount,
        Tile4::kUnitCount + 1,

        Tile8::kUnitCount - 1,
        Tile8::kUnitCount,
        Tile8::kUnitCount + 1,

        Tile8::kUnitCount + Tile1::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount,

        Tile8::kUnitCount * 2 - 1,
        Tile8::kUnitCount * 2,
        Tile8::kUnitCount * 2 + 1,

        Tile8::kUnitCount * 4 + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount
    };

    for (const std::size_t size : sizes) {
        std::vector<char16_t> input(size, u'A');

        if (size >= 2) {
            input[size / 2 - 1] = utf16PairMax.high;
            input[size / 2] = utf16PairMax.low;
        }

        CAPTURE(size);

        const auto simdResult = Utf16SimdKernel::validate(input);
        const auto scalarResult = Utf16Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

// -----------------------------------------------------------------------------
// Block seven: dense valid scheduler workloads
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel validates dense surrogate workload across repeated Tile8 blocks",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::size_t tileCount = 4;
    std::vector<char16_t> input(Tile::kUnitCount * tileCount);

    for (std::size_t i = 0; i < input.size(); i += 2) {
        input[i] = utf16HighMin;
        input[i + 1] = utf16LowMin;
    }

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf16SimdKernel validates mixed BMP and surrogate workload across repeated Tile8 blocks",
          "[core][text][utf16][simd][kernel]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::size_t tileCount = 4;
    std::vector<char16_t> input(Tile::kUnitCount * tileCount);

    for (std::size_t i = 0; i < input.size(); i += 4) {
        input[i] = u'A';
        input[i + 1] = u'B';
        input[i + 2] = utf16HighMin;
        input[i + 3] = utf16LowMin;
    }

    const auto result = Utf16SimdKernel::validate(input);

    REQUIRE(result.status == Utf16Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

// -----------------------------------------------------------------------------
// Block eight: codec surface
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdKernel decode matches Utf16Kernel",
          "[core][text][utf16][simd][kernel]")
{
    constexpr std::array<char16_t, 5> input{
        u'A',
        utf16PairMin.high,
        utf16PairMin.low,
        u'B',
        u'C'
    };

    std::array<UnicodeScalar, 4> simdOutput{};
    std::array<UnicodeScalar, 4> scalarOutput{};

    const auto simdResult = Utf16SimdKernel::decode(input, simdOutput);
    const auto scalarResult = Utf16Kernel::decode(input, scalarOutput);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);

    for (std::size_t i = 0; i < simdResult.outputProduced; ++i)
        REQUIRE(simdOutput[i].value() == scalarOutput[i].value());
}

TEST_CASE("Utf16SimdKernel encode matches Utf16Kernel",
          "[core][text][utf16][simd][kernel]")
{
    constexpr std::array<char32_t, 4> values{
        U'A',
        0x10000,
        U'B',
        0x10FFFF
    };

    std::array<UnicodeScalar, values.size()> input{};

    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto scalar = UnicodeScalar::from(values[i]);
        REQUIRE(scalar.has_value());
        input[i] = *scalar;
    }

    std::array<char16_t, 8> simdOutput{};
    std::array<char16_t, 8> scalarOutput{};

    const auto simdResult = Utf16SimdKernel::encode(input, simdOutput);
    const auto scalarResult = Utf16Kernel::encode(input, scalarOutput);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);

    for (std::size_t i = 0; i < simdResult.outputProduced; ++i)
        REQUIRE(simdOutput[i] == scalarOutput[i]);
}

} // namespace job::core::text::tests