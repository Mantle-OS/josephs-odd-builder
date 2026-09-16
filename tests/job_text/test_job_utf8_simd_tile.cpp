#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include <job_utf8_simd_tile.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: geometry / basic behavior
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile exposes expected tile geometry", "[core][text][utf8][simd][tile]")
{
    STATIC_REQUIRE(Utf8SimdTile<1>::kRegisterCount == 1);
    STATIC_REQUIRE(Utf8SimdTile<2>::kRegisterCount == 2);
    STATIC_REQUIRE(Utf8SimdTile<4>::kRegisterCount == 4);

    STATIC_REQUIRE(Utf8SimdTile<1>::kByteCount == Utf8SimdTile<1>::kRegisterBytes);
    STATIC_REQUIRE(Utf8SimdTile<2>::kByteCount == Utf8SimdTile<2>::kRegisterBytes * 2);
    STATIC_REQUIRE(Utf8SimdTile<4>::kByteCount == Utf8SimdTile<4>::kRegisterBytes * 4);

    STATIC_REQUIRE(Utf8SimdTile<1>::kRegisterBytes == Utf8SimdTile<2>::kRegisterBytes);
    STATIC_REQUIRE(Utf8SimdTile<2>::kRegisterBytes == Utf8SimdTile<4>::kRegisterBytes);
}

TEST_CASE("Utf8SimdTile validates complete ASCII tiles", "[core][text][utf8][simd][tile]")
{
    {
        using Tile = Utf8SimdTile<1>;

        const auto input = makeAsciiTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kByteCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.bytesValidated == Tile::kByteCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf8SimdTile<2>;

        const auto input = makeAsciiTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kByteCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.bytesValidated == Tile::kByteCount);
        REQUIRE(result.state.empty());
    }

    {
        using Tile = Utf8SimdTile<4>;

        const auto input = makeAsciiTile<Tile>();
        const Tile::Input tile{input.data(), Tile::kByteCount};

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.bytesValidated == Tile::kByteCount);
        REQUIRE(result.state.empty());
    }
}

// -----------------------------------------------------------------------------
// Block two: ordinary UTF-8 inside one SIMD register
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile validates two-byte scalars at every in-register offset",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    for (std::size_t offset = 0; offset + utf8Valid2.size() <= Tile::kByteCount; ++offset) {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, offset, utf8Valid2);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.bytesValidated == Tile::kByteCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf8SimdTile validates three-byte scalars at every in-register offset",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    for (std::size_t offset = 0; offset + utf8Valid3.size() <= Tile::kByteCount; ++offset) {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, offset, utf8Valid3);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.bytesValidated == Tile::kByteCount);
        REQUIRE(result.state.empty());
    }
}

TEST_CASE("Utf8SimdTile validates four-byte scalars at every in-register offset",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    for (std::size_t offset = 0; offset + utf8Valid4.size() <= Tile::kByteCount; ++offset) {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, offset, utf8Valid4);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.bytesValidated == Tile::kByteCount);
        REQUIRE(result.state.empty());
    }
}

// -----------------------------------------------------------------------------
// Block three: malformed byte classes
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile rejects lone continuation bytes at every register offset",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    for (std::size_t offset = 0; offset < Tile::kByteCount; ++offset) {
        auto input = makeAsciiTile<Tile>();
        input[offset] = 0x80;

        const Tile::Input tile{input.data(), Tile::kByteCount};

        CAPTURE(offset);

        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.bytesValidated == 0);
    }
}

TEST_CASE("Utf8SimdTile rejects invalid UTF-8 lead bytes",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    constexpr std::array<std::uint8_t, 4> invalidLeads{
        0xC0,
        0xC1,
        0xF5,
        0xFF
    };

    for (const std::uint8_t invalid : invalidLeads) {
        for (std::size_t offset = 0; offset < Tile::kByteCount; ++offset) {
            auto input = makeAsciiTile<Tile>();
            input[offset] = invalid;

            const Tile::Input tile{input.data(), Tile::kByteCount};

            CAPTURE(invalid);
            CAPTURE(offset);

            const auto result = Tile::validate(tile);

            REQUIRE(result.status == Tile::Status::Invalid);
            REQUIRE(result.bytesValidated == 0);
        }
    }
}

// -----------------------------------------------------------------------------
// Block four: exceptional first-continuation ranges
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile enforces E0 first-continuation range",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8E0Valid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8E0Invalid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf8SimdTile enforces ED first-continuation range",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8EdValid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8EdInvalid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf8SimdTile enforces F0 first-continuation range",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8F0Valid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8F0Invalid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf8SimdTile enforces F4 first-continuation range",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8F4Valid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, 7, utf8F4Invalid);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Invalid);
    }
}

// -----------------------------------------------------------------------------
// Block five: outgoing register state
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile carries two-byte sequence state across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto input = makeAsciiTile<Tile>();
    input[Tile::kByteCount - 1] = 0xC2;

    const Tile::Input tile{input.data(), Tile::kByteCount};
    const auto result = Tile::validate(tile);

    REQUIRE(result.status == Tile::Status::Valid);
    REQUIRE(result.bytesValidated == Tile::kByteCount);
    REQUIRE(result.state.expectedContinuationMask == 0x01);
    REQUIRE(result.state.trailingSequenceBytes == 1);
    REQUIRE(result.state.firstContinuationRule == Tile::FirstContinuationRule::Any);
}

TEST_CASE("Utf8SimdTile carries three-byte sequence state across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    SECTION("lead byte is final byte")
    {
        auto input = makeAsciiTile<Tile>();
        input[Tile::kByteCount - 1] = 0xE2;

        const Tile::Input tile{input.data(), Tile::kByteCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.state.expectedContinuationMask == 0x03);
        REQUIRE(result.state.trailingSequenceBytes == 1);
    }

    SECTION("one continuation byte is already consumed")
    {
        auto input = makeAsciiTile<Tile>();

        input[Tile::kByteCount - 2] = 0xE2;
        input[Tile::kByteCount - 1] = 0x82;

        const Tile::Input tile{input.data(), Tile::kByteCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.state.expectedContinuationMask == 0x01);
        REQUIRE(result.state.trailingSequenceBytes == 2);
    }
}

TEST_CASE("Utf8SimdTile carries four-byte sequence state across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    SECTION("lead byte is final byte")
    {
        auto input = makeAsciiTile<Tile>();
        input[Tile::kByteCount - 1] = 0xF0;

        const Tile::Input tile{input.data(), Tile::kByteCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.state.expectedContinuationMask == 0x07);
        REQUIRE(result.state.trailingSequenceBytes == 1);
        REQUIRE(result.state.firstContinuationRule == Tile::FirstContinuationRule::Ninety_BF);
    }

    SECTION("one continuation byte is already consumed")
    {
        auto input = makeAsciiTile<Tile>();

        input[Tile::kByteCount - 2] = 0xF0;
        input[Tile::kByteCount - 1] = 0x9F;

        const Tile::Input tile{input.data(), Tile::kByteCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.state.expectedContinuationMask == 0x03);
        REQUIRE(result.state.trailingSequenceBytes == 2);
        REQUIRE(result.state.firstContinuationRule == Tile::FirstContinuationRule::Any);
    }

    SECTION("two continuation bytes are already consumed")
    {
        auto input = makeAsciiTile<Tile>();

        input[Tile::kByteCount - 3] = 0xF0;
        input[Tile::kByteCount - 2] = 0x9F;
        input[Tile::kByteCount - 1] = 0x98;

        const Tile::Input tile{input.data(), Tile::kByteCount};
        const auto result = Tile::validate(tile);

        REQUIRE(result.status == Tile::Status::Valid);
        REQUIRE(result.state.expectedContinuationMask == 0x01);
        REQUIRE(result.state.trailingSequenceBytes == 3);
        REQUIRE(result.state.firstContinuationRule == Tile::FirstContinuationRule::Any);
    }
}

// -----------------------------------------------------------------------------
// Block six: explicit Tile<1> state chaining
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile completes a two-byte scalar across chained registers",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    auto second = makeAsciiTile<Tile>();

    first[Tile::kByteCount - 1] = 0xC2;
    second[0] = 0xA2;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const Tile::Input secondTile{second.data(), Tile::kByteCount};

    const auto firstResult = Tile::validate(firstTile);

    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE_FALSE(firstResult.state.empty());

    const auto secondResult = Tile::validate(secondTile, firstResult.state);

    REQUIRE(secondResult.status == Tile::Status::Valid);
    REQUIRE(secondResult.bytesValidated == Tile::kByteCount);
    REQUIRE(secondResult.state.empty());
}

TEST_CASE("Utf8SimdTile completes a three-byte scalar across chained registers",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    auto second = makeAsciiTile<Tile>();

    first[Tile::kByteCount - 2] = 0xE2;
    first[Tile::kByteCount - 1] = 0x82;
    second[0] = 0xAC;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const Tile::Input secondTile{second.data(), Tile::kByteCount};

    const auto firstResult = Tile::validate(firstTile);
    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE(firstResult.state.expectedContinuationMask == 0x01);

    const auto secondResult = Tile::validate(secondTile, firstResult.state);

    REQUIRE(secondResult.status == Tile::Status::Valid);
    REQUIRE(secondResult.state.empty());
}

TEST_CASE("Utf8SimdTile completes a four-byte scalar across chained registers",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    auto second = makeAsciiTile<Tile>();

    first[Tile::kByteCount - 1] = 0xF0;

    second[0] = 0x9F;
    second[1] = 0x98;
    second[2] = 0x80;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const Tile::Input secondTile{second.data(), Tile::kByteCount};

    const auto firstResult = Tile::validate(firstTile);

    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE(firstResult.state.expectedContinuationMask == 0x07);
    REQUIRE(firstResult.state.firstContinuationRule == Tile::FirstContinuationRule::Ninety_BF);

    const auto secondResult = Tile::validate(secondTile, firstResult.state);

    REQUIRE(secondResult.status == Tile::Status::Valid);
    REQUIRE(secondResult.state.empty());
}

// -----------------------------------------------------------------------------
// Block seven: exceptional rules across register boundaries
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile enforces E0 range across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    first.back() = 0xE0;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const auto firstResult = Tile::validate(firstTile);

    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE(firstResult.state.firstContinuationRule == Tile::FirstContinuationRule::A0_BF);

    SECTION("A0 is accepted")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0xA0;
        second[1] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Valid);
    }

    SECTION("9F is rejected")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0x9F;
        second[1] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf8SimdTile enforces ED range across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    first.back() = 0xED;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const auto firstResult = Tile::validate(firstTile);

    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE(firstResult.state.firstContinuationRule == Tile::FirstContinuationRule::Eighty_9F);

    SECTION("9F is accepted")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0x9F;
        second[1] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Valid);
    }

    SECTION("A0 is rejected")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0xA0;
        second[1] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf8SimdTile enforces F0 range across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    first.back() = 0xF0;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const auto firstResult = Tile::validate(firstTile);

    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE(firstResult.state.firstContinuationRule == Tile::FirstContinuationRule::Ninety_BF);

    SECTION("90 is accepted")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0x90;
        second[1] = 0x80;
        second[2] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Valid);
    }

    SECTION("8F is rejected")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0x8F;
        second[1] = 0x80;
        second[2] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Invalid);
    }
}

TEST_CASE("Utf8SimdTile enforces F4 range across a register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<1>;

    auto first = makeAsciiTile<Tile>();
    first.back() = 0xF4;

    const Tile::Input firstTile{first.data(), Tile::kByteCount};
    const auto firstResult = Tile::validate(firstTile);

    REQUIRE(firstResult.status == Tile::Status::Valid);
    REQUIRE(firstResult.state.firstContinuationRule == Tile::FirstContinuationRule::Eighty_8F);

    SECTION("8F is accepted")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0x8F;
        second[1] = 0x80;
        second[2] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Valid);
    }

    SECTION("90 is rejected")
    {
        auto second = makeAsciiTile<Tile>();
        second[0] = 0x90;
        second[1] = 0x80;
        second[2] = 0x80;

        const Tile::Input secondTile{second.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(secondTile, firstResult.state).status == Tile::Status::Invalid);
    }
}

// -----------------------------------------------------------------------------
// Block eight: multi-register tile propagation
// -----------------------------------------------------------------------------

TEST_CASE("Utf8SimdTile validates scalars spanning internal Tile2 register boundary",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<2>;

    constexpr std::array<std::size_t, 3> offsets{
        Tile::kRegisterBytes - 1,
        Tile::kRegisterBytes - 2,
        Tile::kRegisterBytes - 3
    };

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, offsets[0], utf8Valid2);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, offsets[1], utf8Valid3);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }

    {
        auto input = makeAsciiTile<Tile>();
        writeBytes<Tile>(input, offsets[2], utf8Valid4);

        const Tile::Input tile{input.data(), Tile::kByteCount};

        REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
    }
}

TEST_CASE("Utf8SimdTile validates scalars across every internal Tile4 register boundary", "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<4>;

    for (std::size_t registerIndex = 1; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        const std::size_t boundary = registerIndex * Tile::kRegisterBytes;

        {
            auto input = makeAsciiTile<Tile>();
            writeBytes<Tile>(input, boundary - 1, utf8Valid2);

            const Tile::Input tile{input.data(), Tile::kByteCount};

            CAPTURE(registerIndex);
            REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
        }

        {
            auto input = makeAsciiTile<Tile>();
            writeBytes<Tile>(input, boundary - 2, utf8Valid3);

            const Tile::Input tile{input.data(), Tile::kByteCount};

            CAPTURE(registerIndex);
            REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
        }

        {
            auto input = makeAsciiTile<Tile>();
            writeBytes<Tile>(input, boundary - 3, utf8Valid4);

            const Tile::Input tile{input.data(), Tile::kByteCount};

            CAPTURE(registerIndex);
            REQUIRE(Tile::validate(tile).status == Tile::Status::Valid);
        }
    }
}

TEST_CASE("Utf8SimdTile reports completed register progress before a later register fails",
          "[core][text][utf8][simd][tile]")
{
    using Tile = Utf8SimdTile<4>;

    for (std::size_t registerIndex = 0; registerIndex < Tile::kRegisterCount; ++registerIndex) {
        auto input = makeAsciiTile<Tile>();

        input[registerIndex * Tile::kRegisterBytes + 7] = 0x80;

        const Tile::Input tile{input.data(), Tile::kByteCount};
        const auto result = Tile::validate(tile);

        CAPTURE(registerIndex);

        REQUIRE(result.status == Tile::Status::Invalid);
        REQUIRE(result.bytesValidated == registerIndex * Tile::kRegisterBytes);
    }
}

} // namespace job::core::text::tests