#include <array>
#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include <job_utf32_simd_tile.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: geometry / experimental extents
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile exposes experimental tile geometry",
          "[core][text][utf32][simd][tile]")
{
    STATIC_REQUIRE(Utf32SimdTile<1>::kRegisterCount == 1);
    STATIC_REQUIRE(Utf32SimdTile<2>::kRegisterCount == 2);
    STATIC_REQUIRE(Utf32SimdTile<4>::kRegisterCount == 4);
    STATIC_REQUIRE(Utf32SimdTile<8>::kRegisterCount == 8);
    STATIC_REQUIRE(Utf32SimdTile<16>::kRegisterCount == 16);

    STATIC_REQUIRE(Utf32SimdTile<1>::kUnitCount == Utf32SimdTile<1>::kRegisterUnits);
    STATIC_REQUIRE(Utf32SimdTile<2>::kUnitCount == Utf32SimdTile<2>::kRegisterUnits * 2);
    STATIC_REQUIRE(Utf32SimdTile<4>::kUnitCount == Utf32SimdTile<4>::kRegisterUnits * 4);
    STATIC_REQUIRE(Utf32SimdTile<8>::kUnitCount == Utf32SimdTile<8>::kRegisterUnits * 8);
    STATIC_REQUIRE(Utf32SimdTile<16>::kUnitCount == Utf32SimdTile<16>::kRegisterUnits * 16);

    STATIC_REQUIRE(Utf32SimdTile<1>::kRegisterUnits == Utf32SimdTile<2>::kRegisterUnits);
    STATIC_REQUIRE(Utf32SimdTile<2>::kRegisterUnits == Utf32SimdTile<4>::kRegisterUnits);
    STATIC_REQUIRE(Utf32SimdTile<4>::kRegisterUnits == Utf32SimdTile<8>::kRegisterUnits);
    STATIC_REQUIRE(Utf32SimdTile<8>::kRegisterUnits == Utf32SimdTile<16>::kRegisterUnits);
}

TEST_CASE("Utf32SimdTile validates complete tiles at every experimental extent",
          "[core][text][utf32][simd][tile]")
{
    {
        using Tile = Utf32SimdTile<1>;

        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<2>;

        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<4>;

        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<8>;

        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<16>;

        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }
}

// -----------------------------------------------------------------------------
// Block two: Unicode scalar boundaries
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile accepts Unicode scalar boundary values",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    constexpr std::array<char32_t, 4> values{
        utf32ScalarMin,
        utf32SurrogateBefore,
        utf32SurrogateAfter,
        utf32ScalarMax
    };

    for (const char32_t value : values) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(value);

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(static_cast<std::uint32_t>(value));

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }
}

TEST_CASE("Utf32SimdTile rejects surrogate boundary values",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    constexpr std::array<char32_t, 2> values{
        utf32SurrogateMin,
        utf32SurrogateMax
    };

    for (const char32_t value : values) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');
        input[3] = value;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(static_cast<std::uint32_t>(value));

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

TEST_CASE("Utf32SimdTile rejects value immediately above Unicode scalar maximum",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    std::array<char32_t, Tile::kUnitCount> input{};
    input.fill(U'A');
    input[3] = utf32AboveScalarMax;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Invalid);
    REQUIRE(result.unitsValidated == 0);
}

TEST_CASE("Utf32SimdTile rejects maximum uint32 value",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    std::array<char32_t, Tile::kUnitCount> input{};
    input.fill(U'A');
    input[3] = static_cast<char32_t>(0xFFFFFFFFu);

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Invalid);
    REQUIRE(result.unitsValidated == 0);
}

// -----------------------------------------------------------------------------
// Block three: every lane
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile rejects surrogate in every register lane",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    for (std::size_t lane = 0; lane < Tile::kRegisterUnits; ++lane) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');
        input[lane] = utf32SurrogateMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(lane);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

TEST_CASE("Utf32SimdTile rejects out of range value in every register lane",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    for (std::size_t lane = 0; lane < Tile::kRegisterUnits; ++lane) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');
        input[lane] = utf32AboveScalarMax;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(lane);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

// -----------------------------------------------------------------------------
// Block four: surrogate classification
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile rejects values throughout surrogate range",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    constexpr std::array<char32_t, 5> values{
        0x0000D800,
        0x0000D801,
        0x0000DBFF,
        0x0000DC00,
        0x0000DFFF
    };

    for (const char32_t value : values) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');
        input[4] = value;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(static_cast<std::uint32_t>(value));

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

TEST_CASE("Utf32SimdTile surrogate mask does not reject neighboring scalar values",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    constexpr std::array<char32_t, 4> values{
        0x0000D7FF,
        0x0000E000,
        0x0001D800,
        0x0010D800
    };

    for (const char32_t value : values) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');
        input[4] = value;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(static_cast<std::uint32_t>(value));

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }
}

// -----------------------------------------------------------------------------
// Block five: unsigned comparison semantics
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile rejects values with the unsigned high bit set",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<1>;

    constexpr std::array<char32_t, 4> values{
        static_cast<char32_t>(0x80000000u),
        static_cast<char32_t>(0x80000001u),
        static_cast<char32_t>(0xFFFFFFFEu),
        static_cast<char32_t>(0xFFFFFFFFu)
    };

    for (const char32_t value : values) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');
        input[5] = value;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(static_cast<std::uint32_t>(value));

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

// -----------------------------------------------------------------------------
// Block six: multi-register progress
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile reports completed register progress before Tile2 failure",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<2>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        input[registerIndex * Tile::kRegisterUnits + 3] = utf32SurrogateMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(registerIndex);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

TEST_CASE("Utf32SimdTile reports completed register progress before Tile4 failure",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<4>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        input[registerIndex * Tile::kRegisterUnits + 3] = utf32AboveScalarMax;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(registerIndex);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

TEST_CASE("Utf32SimdTile reports completed register progress before Tile8 failure",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<8>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        input[registerIndex * Tile::kRegisterUnits + 3] = utf32SurrogateMax;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(registerIndex);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

TEST_CASE("Utf32SimdTile reports completed register progress before Tile16 failure",
          "[core][text][utf32][simd][tile]")
{
    using Tile = Utf32SimdTile<16>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        std::array<char32_t, Tile::kUnitCount> input{};
        input.fill(U'A');

        input[registerIndex * Tile::kRegisterUnits + 3] = utf32AboveScalarMax;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(registerIndex);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

// -----------------------------------------------------------------------------
// Block seven: dense valid values
// -----------------------------------------------------------------------------

TEST_CASE("Utf32SimdTile validates dense mixed scalar values at every experimental extent",
          "[core][text][utf32][simd][tile]")
{
    {
        using Tile = Utf32SimdTile<1>;

        std::array<char32_t, Tile::kUnitCount> input{};

        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = pattern[i % pattern.size()];

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<2>;

        std::array<char32_t, Tile::kUnitCount> input{};

        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = pattern[i % pattern.size()];

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<4>;

        std::array<char32_t, Tile::kUnitCount> input{};

        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = pattern[i % pattern.size()];

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<8>;

        std::array<char32_t, Tile::kUnitCount> input{};

        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = pattern[i % pattern.size()];

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }

    {
        using Tile = Utf32SimdTile<16>;

        std::array<char32_t, Tile::kUnitCount> input{};

        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < input.size(); ++i)
            input[i] = pattern[i % pattern.size()];

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
    }
}

} // namespace job::core::text::tests