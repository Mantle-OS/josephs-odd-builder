#include <array>

#include <catch2/catch_test_macros.hpp>

#include <job_unicode_scalar.h>
#include <job_utf32_scalar_kernel.h>
#include <job_utf32_status.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf32ScalarKernel decodes UTF-32 units", "[core][text][utf32][scalar][usage]")
{
    constexpr std::array<char32_t, 1> input{U'A'};

    UnicodeScalar scalar{};
    const auto result = Utf32ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(scalar.value() == U'A');
}

TEST_CASE("Utf32ScalarKernel decodes supplementary Unicode scalars", "[core][text][utf32][scalar][usage]")
{
    constexpr std::array<char32_t, 1> input{0x1F600};

    UnicodeScalar scalar{};
    const auto result = Utf32ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(scalar.value() == 0x1F600);
}

TEST_CASE("Utf32ScalarKernel encodes Unicode scalars", "[core][text][utf32][scalar][usage]")
{
    const auto scalar = UnicodeScalar::from(0x1F680);
    REQUIRE(scalar.has_value());

    std::array<char32_t, 1> output{};

    const auto result = Utf32ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0] == 0x1F680);
}

TEST_CASE("Utf32ScalarKernel validates UTF-32 units", "[core][text][utf32][scalar][usage]")
{
    constexpr std::array<char32_t, 1> input{0x20AC};

    const auto result = Utf32ScalarKernel::validate(input);

    CHECK(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf32ScalarKernel decodes UTF-32 scalar boundary values", "[core][text][utf32][scalar][edge]")
{
    constexpr std::array fixtures{
        Utf32BoundaryFixture{0x000000, 0x000000},
        Utf32BoundaryFixture{0x00D7FF, 0x00D7FF},
        Utf32BoundaryFixture{0x00E000, 0x00E000},
        Utf32BoundaryFixture{0x00FFFF, 0x00FFFF},
        Utf32BoundaryFixture{0x010000, 0x010000},
        Utf32BoundaryFixture{0x10FFFF, 0x10FFFF}
    };

    for (const auto &fixture : fixtures) {
        const std::array<char32_t, 1> input{fixture.unit};

        UnicodeScalar scalar{};
        const auto result = Utf32ScalarKernel::decode(input, scalar);

        CAPTURE(fixture.unit, fixture.expected);

        REQUIRE(result.status == Utf32Status::Complete);
        CHECK(result.inputConsumed == 1);
        CHECK(result.outputProduced == 1);
        CHECK(scalar.value() == fixture.expected);
    }
}

TEST_CASE("Utf32ScalarKernel rejects surrogate values", "[core][text][utf32][scalar][edge]")
{
    constexpr std::array<char32_t, 4> invalid{
        0xD800,
        0xDBFF,
        0xDC00,
        0xDFFF
    };

    for (const auto value : invalid) {
        const std::array<char32_t, 1> input{value};

        UnicodeScalar scalar{};
        const auto result = Utf32ScalarKernel::decode(input, scalar);

        CAPTURE(value);

        CHECK(result.status == Utf32Status::Invalid);
        CHECK(result.inputConsumed == 0);
        CHECK(result.outputProduced == 0);
    }
}

TEST_CASE("Utf32ScalarKernel rejects values above U+10FFFF", "[core][text][utf32][scalar][edge]")
{
    constexpr std::array<char32_t, 3> invalid{
        0x110000,
        0x1FFFFF,
        static_cast<char32_t>(0xFFFFFFFF)
    };

    for (const auto value : invalid) {
        const std::array<char32_t, 1> input{value};

        UnicodeScalar scalar{};
        const auto result = Utf32ScalarKernel::decode(input, scalar);

        CAPTURE(value);

        CHECK(result.status == Utf32Status::Invalid);
        CHECK(result.inputConsumed == 0);
        CHECK(result.outputProduced == 0);
    }
}

TEST_CASE("Utf32ScalarKernel reports empty input as incomplete", "[core][text][utf32][scalar][edge]")
{
    constexpr std::array<char32_t, 0> input{};

    UnicodeScalar scalar{};
    const auto result = Utf32ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf32Status::Incomplete);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf32ScalarKernel reports output exhaustion", "[core][text][utf32][scalar][edge][output-full]")
{
    const auto scalar = UnicodeScalar::from(0x1F600);
    REQUIRE(scalar.has_value());

    std::array<char32_t, 0> output{};

    const auto result = Utf32ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf32Status::OutputFull);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf32ScalarKernel encode and decode round-trip scalar boundaries",
          "[core][text][utf32][scalar][edge][roundtrip]")
{
    constexpr std::array<char32_t, 8> values{
        0x000000,
        0x00007F,
        0x000080,
        0x00D7FF,
        0x00E000,
        0x00FFFF,
        0x010000,
        0x10FFFF
    };

    for (const auto value : values) {
        const auto input = UnicodeScalar::from(value);
        REQUIRE(input.has_value());

        std::array<char32_t, 1> encoded{};
        const auto encodeResult = Utf32ScalarKernel::encode(*input, encoded);

        CAPTURE(value);

        REQUIRE(encodeResult.status == Utf32Status::Complete);
        CHECK(encodeResult.inputConsumed == 1);
        CHECK(encodeResult.outputProduced == 1);
        CHECK(encoded[0] == value);

        UnicodeScalar decoded{};
        const auto decodeResult = Utf32ScalarKernel::decode(encoded, decoded);

        REQUIRE(decodeResult.status == Utf32Status::Complete);
        CHECK(decodeResult.inputConsumed == 1);
        CHECK(decodeResult.outputProduced == 1);
        CHECK(decoded.value() == value);
    }
}

TEST_CASE("Utf32ScalarKernel operations are constexpr", "[core][text][utf32][scalar][constexpr]")
{
    constexpr auto encoded = [] {
        constexpr auto scalar = UnicodeScalar::from(0x1F680);
        static_assert(scalar.has_value());

        std::array<char32_t, 1> output{};
        const auto result = Utf32ScalarKernel::encode(*scalar, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(encoded.second.status == Utf32Status::Complete);
    STATIC_CHECK(encoded.second.inputConsumed == 1);
    STATIC_CHECK(encoded.second.outputProduced == 1);
    STATIC_CHECK(encoded.first[0] == 0x1F680);

    constexpr auto decoded = [] {
        constexpr std::array<char32_t, 1> input{0x10FFFF};

        UnicodeScalar scalar{};
        const auto result = Utf32ScalarKernel::decode(input, scalar);

        return std::pair{scalar, result};
    }();

    STATIC_CHECK(decoded.second.status == Utf32Status::Complete);
    STATIC_CHECK(decoded.second.inputConsumed == 1);
    STATIC_CHECK(decoded.second.outputProduced == 1);
    STATIC_CHECK(decoded.first.value() == 0x10FFFF);
}

} // namespace job::core::text::tests