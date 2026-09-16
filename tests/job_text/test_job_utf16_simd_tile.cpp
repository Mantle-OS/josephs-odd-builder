#include <array>
#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include <job_utf16_simd_tile.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: geometry / supported extents
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdTile exposes supported tile geometry",
          "[core][text][utf16][simd][tile]")
{
    STATIC_REQUIRE(Utf16SimdTile<1>::kRegisterCount == 1);
    STATIC_REQUIRE(Utf16SimdTile<2>::kRegisterCount == 2);
    STATIC_REQUIRE(Utf16SimdTile<4>::kRegisterCount == 4);
    STATIC_REQUIRE(Utf16SimdTile<8>::kRegisterCount == 8);

    STATIC_REQUIRE(Utf16SimdTile<1>::kUnitCount == Utf16SimdTile<1>::kRegisterUnits);
    STATIC_REQUIRE(Utf16SimdTile<2>::kUnitCount == Utf16SimdTile<2>::kRegisterUnits * 2);
    STATIC_REQUIRE(Utf16SimdTile<4>::kUnitCount == Utf16SimdTile<4>::kRegisterUnits * 4);
    STATIC_REQUIRE(Utf16SimdTile<8>::kUnitCount == Utf16SimdTile<8>::kRegisterUnits * 8);

    STATIC_REQUIRE(Utf16SimdTile<1>::kRegisterUnits == Utf16SimdTile<2>::kRegisterUnits);
    STATIC_REQUIRE(Utf16SimdTile<2>::kRegisterUnits == Utf16SimdTile<4>::kRegisterUnits);
    STATIC_REQUIRE(Utf16SimdTile<4>::kRegisterUnits == Utf16SimdTile<8>::kRegisterUnits);
}

TEST_CASE("Utf16SimdTile validates complete BMP tiles at every supported extent",
          "[core][text][utf16][simd][tile]")
{
    {
        using Tile = Utf16SimdTile<1>;

        const auto input = makeBmpTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kUnitCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf16SimdTile<2>;

        const auto input = makeBmpTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kUnitCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf16SimdTile<4>;

        const auto input = makeBmpTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kUnitCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf16SimdTile<8>;

        const auto input = makeBmpTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kUnitCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }
}

// -----------------------------------------------------------------------------
// Block two: single-register surrogate classification
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdTile validates minimum surrogate pair at every in-register offset",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();
        writeUtf16Pair<Tile>(input, offset, utf16PairMin.high, utf16PairMin.low);

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf16SimdTile validates maximum surrogate pair at every in-register offset",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();
        writeUtf16Pair<Tile>(input, offset, utf16PairMax.high, utf16PairMax.low);

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf16SimdTile rejects lone high surrogates inside one register",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();
        input[offset] = utf16HighMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

TEST_CASE("Utf16SimdTile rejects lone low surrogates inside one register",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();
        input[offset] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == 0);
    }
}

TEST_CASE("Utf16SimdTile rejects consecutive high surrogates",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();

        input[offset] = utf16HighMin;
        input[offset + 1] = utf16HighMax;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf16SimdTile rejects consecutive low surrogates",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();

        input[offset] = utf16LowMin;
        input[offset + 1] = utf16LowMax;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf16SimdTile rejects BMP units after high surrogates",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();

        input[offset] = utf16HighMin;
        input[offset + 1] = u'A';

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf16SimdTile rejects low surrogates after BMP units",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    for (std::size_t offset = 0; offset + 1 < Tile::kUnitCount; ++offset) {
        auto input = makeBmpTile<Tile>();

        input[offset] = u'A';
        input[offset + 1] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(offset);

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

// -----------------------------------------------------------------------------
// Block three: external state propagation
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdTile carries a trailing high surrogate as outgoing state",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    auto input = makeBmpTile<Tile>();
    input[Tile::kUnitCount - 1] = utf16HighMin;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Valid);
    REQUIRE(result.unitsValidated == Tile::kUnitCount);
    REQUIRE(result.state.expectsLowSurrogate);
}

TEST_CASE("Utf16SimdTile completes a surrogate pair from incoming state",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    auto input = makeBmpTile<Tile>();
    input[0] = utf16LowMin;

    Tile::State state{};
    state.expectsLowSurrogate = true;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile, state);

    REQUIRE(result.status == Tile::Status::Valid);
    REQUIRE(result.unitsValidated == Tile::kUnitCount);
    REQUIRE(result.state.empty());
}

TEST_CASE("Utf16SimdTile rejects BMP unit when incoming state expects a low surrogate",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    const auto input = makeBmpTile<Tile>();

    Tile::State state{};
    state.expectsLowSurrogate = true;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile, state);

    REQUIRE(result.status == Tile::Status::Invalid);
    REQUIRE(result.unitsValidated == 0);
}

TEST_CASE("Utf16SimdTile rejects high surrogate when incoming state expects a low surrogate",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<1>;

    auto input = makeBmpTile<Tile>();
    input[0] = utf16HighMax;

    Tile::State state{};
    state.expectsLowSurrogate = true;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile, state);

    REQUIRE(result.status == Tile::Status::Invalid);
    REQUIRE(result.unitsValidated == 0);
}

// -----------------------------------------------------------------------------
// Block four: internal multi-register state propagation
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdTile validates surrogate pair across Tile2 register boundary",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<2>;

    auto input = makeBmpTile<Tile>();

    const std::size_t boundary = Tile::kRegisterUnits;
    input[boundary - 1] = utf16HighMin;
    input[boundary] = utf16LowMin;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Valid);
    REQUIRE(result.unitsValidated == Tile::kUnitCount);
    REQUIRE(result.state.empty());
}

TEST_CASE("Utf16SimdTile validates surrogate pairs across every Tile4 register boundary",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<4>;

    for (std::size_t registerIndex = 1; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        auto input = makeBmpTile<Tile>();

        const std::size_t boundary = registerIndex * Tile::kRegisterUnits;
        input[boundary - 1] = utf16HighMin;
        input[boundary] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(registerIndex);
        CAPTURE(boundary);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf16SimdTile validates surrogate pairs across every Tile8 register boundary",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<8>;

    for (std::size_t registerIndex = 1; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        auto input = makeBmpTile<Tile>();

        const std::size_t boundary = registerIndex * Tile::kRegisterUnits;
        input[boundary - 1] = utf16HighMin;
        input[boundary] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};

        CAPTURE(registerIndex);
        CAPTURE(boundary);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf16SimdTile propagates trailing high surrogate through Tile8",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<8>;

    auto input = makeBmpTile<Tile>();
    input[Tile::kUnitCount - 1] = utf16HighMin;

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Valid);
    REQUIRE(result.unitsValidated == Tile::kUnitCount);
    REQUIRE(result.state.expectsLowSurrogate);
}

// -----------------------------------------------------------------------------
// Block five: dense valid surrogate patterns
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdTile validates dense surrogate pairs at every supported extent",
          "[core][text][utf16][simd][tile]")
{
    {
        using Tile = Utf16SimdTile<1>;

        std::array<char16_t, Tile::kUnitCount> input{};

        for (std::size_t i = 0; i < input.size(); i += 2) {
            input[i] = utf16HighMin;
            input[i + 1] = utf16LowMin;
        }

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf16SimdTile<2>;

        std::array<char16_t, Tile::kUnitCount> input{};

        for (std::size_t i = 0; i < input.size(); i += 2) {
            input[i] = utf16HighMin;
            input[i + 1] = utf16LowMin;
        }

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf16SimdTile<4>;

        std::array<char16_t, Tile::kUnitCount> input{};

        for (std::size_t i = 0; i < input.size(); i += 2) {
            input[i] = utf16HighMin;
            input[i + 1] = utf16LowMin;
        }

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf16SimdTile<8>;

        std::array<char16_t, Tile::kUnitCount> input{};

        for (std::size_t i = 0; i < input.size(); i += 2) {
            input[i] = utf16HighMin;
            input[i + 1] = utf16LowMin;
        }

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.unitsValidated == Tile::kUnitCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf16SimdTile validates mixed BMP and surrogate pattern in Tile8",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<8>;

    std::array<char16_t, Tile::kUnitCount> input{};

    for (std::size_t i = 0; i < input.size(); i += 4) {
        input[i] = u'A';
        input[i + 1] = u'B';
        input[i + 2] = utf16HighMin;
        input[i + 3] = utf16LowMin;
    }

    const Tile::Input tile{input.data(), Tile::kUnitCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Valid);
    REQUIRE(result.unitsValidated == Tile::kUnitCount);
    REQUIRE(result.state.empty());
}

// -----------------------------------------------------------------------------
// Block six: completed-register progress before failure
// -----------------------------------------------------------------------------

TEST_CASE("Utf16SimdTile reports completed register progress before Tile2 failure",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<2>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        auto input = makeBmpTile<Tile>();

        input[registerIndex * Tile::kRegisterUnits + 7] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        CAPTURE(registerIndex);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

TEST_CASE("Utf16SimdTile reports completed register progress before Tile4 failure",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<4>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        auto input = makeBmpTile<Tile>();

        input[registerIndex * Tile::kRegisterUnits + 7] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        CAPTURE(registerIndex);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

TEST_CASE("Utf16SimdTile reports completed register progress before Tile8 failure",
          "[core][text][utf16][simd][tile]")
{
    using Tile = Utf16SimdTile<8>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        auto input = makeBmpTile<Tile>();

        input[registerIndex * Tile::kRegisterUnits + 7] = utf16LowMin;

        const Tile::Input tile{input.data(), Tile::kUnitCount};
        const auto result = Tile::validate(tile);

        CAPTURE(registerIndex);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.unitsValidated == registerIndex * Tile::kRegisterUnits);
    }
}

} // namespace job::core::text::tests