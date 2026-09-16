#include <array>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include <job_unicode_scalar.h>
#include <job_utf8_scalar_kernel.h>
#include <job_utf8_status.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf8ScalarKernel decodes ASCII", "[core][text][utf8][scalar][usage]")
{
    constexpr std::array<std::uint8_t, 1> input{0x41};

    UnicodeScalar scalar{};
    const auto result = Utf8ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(scalar.value() == U'A');
}

TEST_CASE("Utf8ScalarKernel decodes multibyte UTF-8", "[core][text][utf8][scalar][usage]")
{
    constexpr std::array<std::uint8_t, 4> input{0xF0, 0x9F, 0x98, 0x80};

    UnicodeScalar scalar{};
    const auto result = Utf8ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == 4);
    CHECK(result.outputProduced == 1);
    CHECK(scalar.value() == 0x1F600);
}

TEST_CASE("Utf8ScalarKernel encodes Unicode scalar values", "[core][text][utf8][scalar][usage]")
{
    const auto scalar = UnicodeScalar::from(0x1F680);
    REQUIRE(scalar.has_value());

    std::array<std::uint8_t, 4> output{};
    const auto result = Utf8ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 4);

    CHECK(output[0] == 0xF0);
    CHECK(output[1] == 0x9F);
    CHECK(output[2] == 0x9A);
    CHECK(output[3] == 0x80);
}

TEST_CASE("Utf8ScalarKernel validates UTF-8 without producing a scalar", "[core][text][utf8][scalar][usage]")
{
    constexpr std::array<std::uint8_t, 3> input{0xE2, 0x82, 0xAC};

    const auto result = Utf8ScalarKernel::validate(input);

    CHECK(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == 3);
    CHECK(result.outputProduced == 1);
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf8ScalarKernel decodes UTF-8 boundary values", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array fixtures{
        Utf8BoundaryFixture{{0x00, 0x00, 0x00, 0x00}, 1, 0x000000},
        Utf8BoundaryFixture{{0x7F, 0x00, 0x00, 0x00}, 1, 0x00007F},
        Utf8BoundaryFixture{{0xC2, 0x80, 0x00, 0x00}, 2, 0x000080},
        Utf8BoundaryFixture{{0xDF, 0xBF, 0x00, 0x00}, 2, 0x0007FF},
        Utf8BoundaryFixture{{0xE0, 0xA0, 0x80, 0x00}, 3, 0x000800},
        Utf8BoundaryFixture{{0xED, 0x9F, 0xBF, 0x00}, 3, 0x00D7FF},
        Utf8BoundaryFixture{{0xEE, 0x80, 0x80, 0x00}, 3, 0x00E000},
        Utf8BoundaryFixture{{0xEF, 0xBF, 0xBF, 0x00}, 3, 0x00FFFF},
        Utf8BoundaryFixture{{0xF0, 0x90, 0x80, 0x80}, 4, 0x010000},
        Utf8BoundaryFixture{{0xF4, 0x8F, 0xBF, 0xBF}, 4, 0x10FFFF}
    };

    for (const auto &fixture : fixtures) {
        UnicodeScalar scalar{};
        const auto result = Utf8ScalarKernel::decode(std::span<const std::uint8_t>{fixture.units.data(), fixture.size},scalar);
        CAPTURE(fixture.size, fixture.expected);
        REQUIRE(result.status == Utf8Status::Complete);
        CHECK(result.inputConsumed == fixture.size);
        CHECK(result.outputProduced == 1);
        CHECK(scalar.value() == fixture.expected);
    }
}

TEST_CASE("Utf8ScalarKernel rejects illegal lead bytes", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array<std::uint8_t, 7> invalid{
        0x80,
        0xBF,
        0xC0,
        0xC1,
        0xF5,
        0xFE,
        0xFF
    };

    for (const auto byte : invalid) {
        const std::array<std::uint8_t, 1> input{byte};

        UnicodeScalar scalar{};
        const auto result = Utf8ScalarKernel::decode(input, scalar);

        CAPTURE(byte);

        CHECK(result.status == Utf8Status::Invalid);
        CHECK(result.inputConsumed == 0);
        CHECK(result.outputProduced == 0);
    }
}

TEST_CASE("Utf8ScalarKernel rejects invalid continuation bytes", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array<std::uint8_t, 3> input{0xE2, 0x28, 0xA1};

    UnicodeScalar scalar{};
    const auto result = Utf8ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf8Status::Invalid);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8ScalarKernel rejects overlong UTF-8 sequences", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array twoByte{std::uint8_t{0xC0}, std::uint8_t{0xAF}};
    constexpr std::array threeByte{std::uint8_t{0xE0}, std::uint8_t{0x80}, std::uint8_t{0xAF}};
    constexpr std::array fourByte{std::uint8_t{0xF0}, std::uint8_t{0x80}, std::uint8_t{0x80}, std::uint8_t{0xAF}};

    UnicodeScalar scalar{};

    CHECK(Utf8ScalarKernel::decode(twoByte, scalar).status == Utf8Status::Invalid);
    CHECK(Utf8ScalarKernel::decode(threeByte, scalar).status == Utf8Status::Invalid);
    CHECK(Utf8ScalarKernel::decode(fourByte, scalar).status == Utf8Status::Invalid);
}

TEST_CASE("Utf8ScalarKernel rejects encoded surrogate values", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array<std::uint8_t, 3> firstSurrogate{0xED, 0xA0, 0x80};
    constexpr std::array<std::uint8_t, 3> lastSurrogate{0xED, 0xBF, 0xBF};

    UnicodeScalar scalar{};

    CHECK(Utf8ScalarKernel::decode(firstSurrogate, scalar).status == Utf8Status::Invalid);
    CHECK(Utf8ScalarKernel::decode(lastSurrogate, scalar).status == Utf8Status::Invalid);
}

TEST_CASE("Utf8ScalarKernel rejects values above U+10FFFF", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array<std::uint8_t, 4> input{0xF4, 0x90, 0x80, 0x80};

    UnicodeScalar scalar{};
    const auto result = Utf8ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf8Status::Invalid);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8ScalarKernel reports incomplete UTF-8 sequences", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array<std::uint8_t, 0> empty{};
    constexpr std::array<std::uint8_t, 1> twoByte{0xC2};
    constexpr std::array<std::uint8_t, 2> threeByte{0xE2, 0x82};
    constexpr std::array<std::uint8_t, 3> fourByte{0xF0, 0x9F, 0x98};

    UnicodeScalar scalar{};

    CHECK(Utf8ScalarKernel::decode(empty, scalar).status == Utf8Status::Incomplete);
    CHECK(Utf8ScalarKernel::decode(twoByte, scalar).status == Utf8Status::Incomplete);
    CHECK(Utf8ScalarKernel::decode(threeByte, scalar).status == Utf8Status::Incomplete);
    CHECK(Utf8ScalarKernel::decode(fourByte, scalar).status == Utf8Status::Incomplete);
}

TEST_CASE("Utf8ScalarKernel reports output exhaustion while encoding", "[core][text][utf8][scalar][edge]")
{
    const auto scalar = UnicodeScalar::from(0x1F600);
    REQUIRE(scalar.has_value());

    std::array<std::uint8_t, 3> output{};

    const auto result = Utf8ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf8Status::OutputFull);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8ScalarKernel encode and decode round-trip scalar boundaries", "[core][text][utf8][scalar][edge]")
{
    constexpr std::array<char32_t, 10> values{
        0x000000,
        0x00007F,
        0x000080,
        0x0007FF,
        0x000800,
        0x00D7FF,
        0x00E000,
        0x00FFFF,
        0x010000,
        0x10FFFF
    };

    for (const auto value : values) {
        const auto input = UnicodeScalar::from(value);
        REQUIRE(input.has_value());

        std::array<std::uint8_t, 4> encoded{};
        const auto encodeResult = Utf8ScalarKernel::encode(*input, encoded);

        CAPTURE(value);

        REQUIRE(encodeResult.status == Utf8Status::Complete);

        UnicodeScalar decoded{};
        const auto decodeResult = Utf8ScalarKernel::decode(
            std::span<const std::uint8_t>{encoded.data(), encodeResult.outputProduced},
            decoded);

        REQUIRE(decodeResult.status == Utf8Status::Complete);
        CHECK(decoded.value() == value);
    }
}

TEST_CASE("Utf8ScalarKernel operations are constexpr", "[core][text][utf8][scalar][constexpr]")
{
    constexpr auto scalar = UnicodeScalar::from(0x20AC);
    STATIC_REQUIRE(scalar.has_value());

    constexpr auto encoded = [] {
        constexpr auto value = UnicodeScalar::from(0x20AC);
        std::array<std::uint8_t, 4> output{};

        const auto result = Utf8ScalarKernel::encode(*value, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(encoded.second.status == Utf8Status::Complete);
    STATIC_CHECK(encoded.second.outputProduced == 3);
    STATIC_CHECK(encoded.first[0] == 0xE2);
    STATIC_CHECK(encoded.first[1] == 0x82);
    STATIC_CHECK(encoded.first[2] == 0xAC);
}

} // namespace job::core::text::tests