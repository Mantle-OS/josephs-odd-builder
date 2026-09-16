#include <array>
#include <cstddef>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <job_utf32_kernel.h>
#include <job_utf32_simd_kernel.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: basic behavior / scalar remainder
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel validates empty input",
          "[core][text][utf32][simd][kernel]")
{
    const std::vector<char32_t> input;

    const auto result = Utf32SimdKernel::validate(input);

    REQUIRE(result.status == Utf32Status::Complete);
    REQUIRE(result.inputConsumed == 0);
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf32SimdKernel validates input smaller than one SIMD register",
          "[core][text][utf32][simd][kernel]")
{
    constexpr std::size_t size = Utf32SimdKernel::kMinimumInputUnits - 1;
    const std::vector<char32_t> input(size, U'A');

    const auto result = Utf32SimdKernel::validate(input);

    REQUIRE(result.status == Utf32Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf32SimdKernel validates exactly one SIMD register",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile1;

    const std::vector<char32_t> input(Tile::kUnitCount, U'A');

    const auto result = Utf32SimdKernel::validate(input);

    REQUIRE(result.status == Utf32Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

// -----------------------------------------------------------------------------
// Block two: experimental scheduler decomposition
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel validates exact SIMD tile decompositions",
          "[core][text][utf32][simd][kernel]")
{
    using Tile1 = Utf32SimdKernel::Tile1;
    using Tile2 = Utf32SimdKernel::Tile2;
    using Tile4 = Utf32SimdKernel::Tile4;
    using Tile8 = Utf32SimdKernel::Tile8;
    using Tile16 = Utf32SimdKernel::Tile16;

    constexpr std::array<std::size_t, 15> sizes{
        Tile1::kUnitCount,

        Tile2::kUnitCount,
        Tile2::kUnitCount + Tile1::kUnitCount,

        Tile4::kUnitCount,
        Tile4::kUnitCount + Tile2::kUnitCount,
        Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount,

        Tile8::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount,
        Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount,

        Tile16::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount + Tile4::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount
    };

    for (const std::size_t size : sizes) {
        const std::vector<char32_t> input(size, U'A');

        CAPTURE(size);

        const auto result = Utf32SimdKernel::validate(input);

        REQUIRE(result.status == Utf32Status::Complete);
        REQUIRE(result.inputConsumed == input.size());
        REQUIRE(result.outputProduced == 0);
    }
}

TEST_CASE("Utf32SimdKernel validates repeated Tile16 input",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::size_t tileCount = 8;
    const std::vector<char32_t> input(Tile::kUnitCount * tileCount, U'A');

    const auto result = Utf32SimdKernel::validate(input);

    REQUIRE(result.status == Utf32Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

TEST_CASE("Utf32SimdKernel validates repeated Tile16 input with scalar tail",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::array<std::size_t, 5> tails{
        1,
        2,
        3,
        Utf32SimdKernel::kMinimumInputUnits - 2,
        Utf32SimdKernel::kMinimumInputUnits - 1
    };

    for (const std::size_t tail : tails) {
        const std::size_t size = Tile::kUnitCount * 2 + tail;
        const std::vector<char32_t> input(size, U'A');

        CAPTURE(tail);
        CAPTURE(size);

        const auto result = Utf32SimdKernel::validate(input);

        REQUIRE(result.status == Utf32Status::Complete);
        REQUIRE(result.inputConsumed == input.size());
        REQUIRE(result.outputProduced == 0);
    }
}

// -----------------------------------------------------------------------------
// Block three: valid scalar boundaries through SIMD scheduler
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel validates Unicode scalar boundary values",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::array<char32_t, 4> pattern{
        utf32ScalarMin,
        utf32SurrogateBefore,
        utf32SurrogateAfter,
        utf32ScalarMax
    };

    std::vector<char32_t> input(Tile::kUnitCount * 2);

    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = pattern[i % pattern.size()];

    const auto result = Utf32SimdKernel::validate(input);

    REQUIRE(result.status == Utf32Status::Complete);
    REQUIRE(result.inputConsumed == input.size());
    REQUIRE(result.outputProduced == 0);
}

// -----------------------------------------------------------------------------
// Block four: malformed input inside Tile16
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel matches scalar for surrogate values in every Tile16 register",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::vector<char32_t> input(Tile::kUnitCount, U'A');

        const std::size_t offset =
            registerIndex * Tile::kRegisterUnits +
            (Tile::kRegisterUnits / 2);

        input[offset] = utf32SurrogateMin;

        CAPTURE(registerIndex);
        CAPTURE(offset);

        const auto simdResult = Utf32SimdKernel::validate(input);
        const auto scalarResult = Utf32Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

TEST_CASE("Utf32SimdKernel matches scalar for out of range values in every Tile16 register",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::vector<char32_t> input(Tile::kUnitCount, U'A');

        const std::size_t offset =
            registerIndex * Tile::kRegisterUnits +
            (Tile::kRegisterUnits / 2);

        input[offset] = utf32AboveScalarMax;

        CAPTURE(registerIndex);
        CAPTURE(offset);

        const auto simdResult = Utf32SimdKernel::validate(input);
        const auto scalarResult = Utf32Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

// -----------------------------------------------------------------------------
// Block five: completed Tile16 blocks before failure
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel matches scalar for malformed input after complete Tile16 blocks",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::size_t prefixTiles = 3;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::vector<char32_t> input(Tile::kUnitCount * (prefixTiles + 1), U'A');

        const std::size_t offset =
            Tile::kUnitCount * prefixTiles +
            registerIndex * Tile::kRegisterUnits +
            (Tile::kRegisterUnits / 2);

        input[offset] = utf32SurrogateMax;

        CAPTURE(registerIndex);
        CAPTURE(offset);

        const auto simdResult = Utf32SimdKernel::validate(input);
        const auto scalarResult = Utf32Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

TEST_CASE("Utf32SimdKernel resumes scalar validation at first unvalidated register",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::size_t failingRegister = 11;

    std::vector<char32_t> input(Tile::kUnitCount, U'A');

    const std::size_t offset =
        failingRegister * Tile::kRegisterUnits +
        (Tile::kRegisterUnits / 2);

    input[offset] = utf32AboveScalarMax;

    const auto simdResult = Utf32SimdKernel::validate(input);
    const auto scalarResult = Utf32Kernel::validate(input);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
}

// -----------------------------------------------------------------------------
// Block six: unsigned comparison semantics through kernel
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel rejects values with unsigned high bit set",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::array<char32_t, 4> values{
        static_cast<char32_t>(0x80000000u),
        static_cast<char32_t>(0x80000001u),
        static_cast<char32_t>(0xFFFFFFFEu),
        static_cast<char32_t>(0xFFFFFFFFu)
    };

    for (const char32_t value : values) {
        std::vector<char32_t> input(Tile::kUnitCount, U'A');
        input[Tile::kRegisterUnits + 3] = value;

        CAPTURE(static_cast<std::uint32_t>(value));

        const auto simdResult = Utf32SimdKernel::validate(input);
        const auto scalarResult = Utf32Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

// -----------------------------------------------------------------------------
// Block seven: scheduler boundary equivalence
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel matches scalar validation across scheduler boundary sizes",
          "[core][text][utf32][simd][kernel]")
{
    using Tile1 = Utf32SimdKernel::Tile1;
    using Tile2 = Utf32SimdKernel::Tile2;
    using Tile4 = Utf32SimdKernel::Tile4;
    using Tile8 = Utf32SimdKernel::Tile8;
    using Tile16 = Utf32SimdKernel::Tile16;

    constexpr std::array<std::size_t, 25> sizes{
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

        Tile16::kUnitCount - 1,
        Tile16::kUnitCount,
        Tile16::kUnitCount + 1,

        Tile16::kUnitCount + Tile8::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount + Tile4::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount,
        Tile16::kUnitCount + Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount,

        Tile16::kUnitCount * 2 - 1,
        Tile16::kUnitCount * 2,
        Tile16::kUnitCount * 2 + 1,

        Tile16::kUnitCount * 4 + Tile8::kUnitCount + Tile4::kUnitCount + Tile2::kUnitCount + Tile1::kUnitCount
    };

    for (const std::size_t size : sizes) {
        std::vector<char32_t> input(size, U'A');

        if (size >= 4) {
            input[size / 4] = utf32SurrogateBefore;
            input[size / 2] = utf32SurrogateAfter;
            input[(size * 3) / 4] = utf32ScalarMax;
        }

        CAPTURE(size);

        const auto simdResult = Utf32SimdKernel::validate(input);
        const auto scalarResult = Utf32Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

// -----------------------------------------------------------------------------
// Block eight: malformed scalar/SIMD equivalence around scheduler thresholds
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel matches scalar for malformed input around Tile16 threshold",
          "[core][text][utf32][simd][kernel]")
{
    using Tile = Utf32SimdKernel::Tile16;

    constexpr std::array<std::size_t, 3> sizes{
        Tile::kUnitCount - 1,
        Tile::kUnitCount,
        Tile::kUnitCount + 1
    };

    for (const std::size_t size : sizes) {
        std::vector<char32_t> input(size, U'A');

        input[size - 1] = utf32AboveScalarMax;

        CAPTURE(size);

        const auto simdResult = Utf32SimdKernel::validate(input);
        const auto scalarResult = Utf32Kernel::validate(input);

        REQUIRE(simdResult.status == scalarResult.status);
        REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
        REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);
    }
}

// -----------------------------------------------------------------------------
// Block nine: codec surface
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdKernel decode matches Utf32Kernel",
          "[core][text][utf32][simd][kernel]")
{
    constexpr std::array<char32_t, 4> input{
        U'A',
        utf32SurrogateBefore,
        utf32SurrogateAfter,
        utf32ScalarMax
    };

    std::array<UnicodeScalar, input.size()> simdOutput{};
    std::array<UnicodeScalar, input.size()> scalarOutput{};

    const auto simdResult = Utf32SimdKernel::decode(input, simdOutput);
    const auto scalarResult = Utf32Kernel::decode(input, scalarOutput);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);

    for (std::size_t i = 0; i < simdResult.outputProduced; ++i)
        REQUIRE(simdOutput[i].value() == scalarOutput[i].value());
}

TEST_CASE("Utf32SimdKernel encode matches Utf32Kernel",
          "[core][text][utf32][simd][kernel]")
{
    constexpr std::array<char32_t, 4> values{
        utf32ScalarMin,
        utf32SurrogateBefore,
        utf32SurrogateAfter,
        utf32ScalarMax
    };

    std::array<UnicodeScalar, values.size()> input{};

    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto scalar = UnicodeScalar::from(values[i]);
        REQUIRE(scalar.has_value());
        input[i] = *scalar;
    }

    std::array<char32_t, values.size()> simdOutput{};
    std::array<char32_t, values.size()> scalarOutput{};

    const auto simdResult = Utf32SimdKernel::encode(input, simdOutput);
    const auto scalarResult = Utf32Kernel::encode(input, scalarOutput);

    REQUIRE(simdResult.status == scalarResult.status);
    REQUIRE(simdResult.inputConsumed == scalarResult.inputConsumed);
    REQUIRE(simdResult.outputProduced == scalarResult.outputProduced);

    for (std::size_t i = 0; i < simdResult.outputProduced; ++i)
        REQUIRE(simdOutput[i] == scalarOutput[i]);
}

} // namespace job::core::text::tests