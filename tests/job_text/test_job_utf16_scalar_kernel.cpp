#include <array>
#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include "job_unicode_scalar.h"
#include "job_utf16_scalar_kernel.h"
#include "job_utf16_status.h"

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf16ScalarKernel decodes BMP Unicode scalars", "[core][text][utf16][scalar][usage]")
{
    constexpr std::array<char16_t, 1> input{u'A'};

    UnicodeScalar scalar{};
    const auto result = Utf16ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(scalar.value() == U'A');
}

TEST_CASE("Utf16ScalarKernel decodes surrogate pairs", "[core][text][utf16][scalar][usage]")
{
    constexpr std::array<char16_t, 2> input{
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE00)
    };

    UnicodeScalar scalar{};
    const auto result = Utf16ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 1);
    CHECK(scalar.value() == 0x1F600);
}

TEST_CASE("Utf16ScalarKernel encodes supplementary Unicode scalars", "[core][text][utf16][scalar][usage]")
{
    const auto scalar = UnicodeScalar::from(0x1F680);
    REQUIRE(scalar.has_value());

    std::array<char16_t, 2> output{};
    const auto result = Utf16ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 2);

    CHECK(output[0] == static_cast<char16_t>(0xD83D));
    CHECK(output[1] == static_cast<char16_t>(0xDE80));
}

TEST_CASE("Utf16ScalarKernel validates UTF-16 scalar sequences", "[core][text][utf16][scalar][usage]")
{
    constexpr std::array<char16_t, 2> input{
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE80)
    };

    const auto result = Utf16ScalarKernel::validate(input);

    CHECK(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 1);
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf16ScalarKernel decodes UTF-16 scalar boundary values", "[core][text][utf16][scalar][edge]")
{
    constexpr std::array fixtures{
        Utf16ScalarBoundaryFixture{{0x0000, 0x0000}, 1, 0x000000},
        Utf16ScalarBoundaryFixture{{0xD7FF, 0x0000}, 1, 0x00D7FF},
        Utf16ScalarBoundaryFixture{{0xE000, 0x0000}, 1, 0x00E000},
        Utf16ScalarBoundaryFixture{{0xFFFF, 0x0000}, 1, 0x00FFFF},
        Utf16ScalarBoundaryFixture{{0xD800, 0xDC00}, 2, 0x010000},
        Utf16ScalarBoundaryFixture{{0xDBFF, 0xDFFF}, 2, 0x10FFFF}
    };

    for (const auto &fixture : fixtures) {
        UnicodeScalar scalar{};

        const auto result = Utf16ScalarKernel::decode(
            std::span<const char16_t>{fixture.units.data(), fixture.size},
            scalar);

        CAPTURE(fixture.size, fixture.expected);

        REQUIRE(result.status == Utf16Status::Complete);
        CHECK(result.inputConsumed == fixture.size);
        CHECK(result.outputProduced == 1);
        CHECK(scalar.value() == fixture.expected);
    }
}

TEST_CASE("Utf16ScalarKernel rejects isolated low surrogates", "[core][text][utf16][scalar][edge]")
{
    constexpr std::array<char16_t, 3> lowSurrogates{
        static_cast<char16_t>(0xDC00),
        static_cast<char16_t>(0xDE00),
        static_cast<char16_t>(0xDFFF)
    };

    for (const auto unit : lowSurrogates) {
        const std::array<char16_t, 1> input{unit};

        UnicodeScalar scalar{};
        const auto result = Utf16ScalarKernel::decode(input, scalar);

        CAPTURE(static_cast<unsigned int>(unit));

        CHECK(result.status == Utf16Status::Invalid);
        CHECK(result.inputConsumed == 0);
        CHECK(result.outputProduced == 0);
    }
}

TEST_CASE("Utf16ScalarKernel reports incomplete high surrogates", "[core][text][utf16][scalar][edge]")
{
    constexpr std::array<char16_t, 1> input{
        static_cast<char16_t>(0xD83D)
    };

    UnicodeScalar scalar{};
    const auto result = Utf16ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf16Status::Incomplete);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16ScalarKernel rejects high surrogate followed by non-low surrogate",
          "[core][text][utf16][scalar][edge]")
{
    constexpr std::array<char16_t, 2> input{
        static_cast<char16_t>(0xD83D),
        u'A'
    };

    UnicodeScalar scalar{};
    const auto result = Utf16ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf16Status::Invalid);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16ScalarKernel rejects high surrogate followed by high surrogate",
          "[core][text][utf16][scalar][edge]")
{
    constexpr std::array<char16_t, 2> input{
        static_cast<char16_t>(0xD800),
        static_cast<char16_t>(0xDBFF)
    };

    UnicodeScalar scalar{};
    const auto result = Utf16ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf16Status::Invalid);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16ScalarKernel reports empty input as incomplete", "[core][text][utf16][scalar][edge]")
{
    constexpr std::array<char16_t, 0> input{};

    UnicodeScalar scalar{};
    const auto result = Utf16ScalarKernel::decode(input, scalar);

    CHECK(result.status == Utf16Status::Incomplete);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16ScalarKernel encodes BMP boundary values", "[core][text][utf16][scalar][edge]")
{
    constexpr std::array<char32_t, 4> values{
        0x000000,
        0x00D7FF,
        0x00E000,
        0x00FFFF
    };

    for (const auto value : values) {
        const auto scalar = UnicodeScalar::from(value);
        REQUIRE(scalar.has_value());

        std::array<char16_t, 1> output{};
        const auto result = Utf16ScalarKernel::encode(*scalar, output);

        CAPTURE(value);

        REQUIRE(result.status == Utf16Status::Complete);
        CHECK(result.inputConsumed == 1);
        CHECK(result.outputProduced == 1);
        CHECK(output[0] == static_cast<char16_t>(value));
    }
}

TEST_CASE("Utf16ScalarKernel encodes supplementary boundary values", "[core][text][utf16][scalar][edge]")
{

    constexpr std::array fixtures{
        Utf16BoundaryFixture{0x010000, 0xD800, 0xDC00},
        Utf16BoundaryFixture{0x10FFFF, 0xDBFF, 0xDFFF}
    };

    for (const auto &fixture : fixtures) {
        const auto scalar = UnicodeScalar::from(fixture.scalar);
        REQUIRE(scalar.has_value());

        std::array<char16_t, 2> output{};
        const auto result = Utf16ScalarKernel::encode(*scalar, output);

        CAPTURE(fixture.scalar);

        REQUIRE(result.status == Utf16Status::Complete);
        CHECK(result.inputConsumed == 1);
        CHECK(result.outputProduced == 2);
        CHECK(output[0] == fixture.high);
        CHECK(output[1] == fixture.low);
    }
}

TEST_CASE("Utf16ScalarKernel reports output exhaustion for BMP encoding",
          "[core][text][utf16][scalar][edge][output-full]")
{
    const auto scalar = UnicodeScalar::from(U'A');
    REQUIRE(scalar.has_value());

    std::array<char16_t, 0> output{};

    const auto result = Utf16ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf16Status::OutputFull);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16ScalarKernel does not write half a surrogate pair",
          "[core][text][utf16][scalar][edge][output-full]")
{
    const auto scalar = UnicodeScalar::from(0x1F600);
    REQUIRE(scalar.has_value());

    std::array<char16_t, 1> output{
        static_cast<char16_t>(0xAAAA)
    };

    const auto result = Utf16ScalarKernel::encode(*scalar, output);

    CHECK(result.status == Utf16Status::OutputFull);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
    CHECK(output[0] == static_cast<char16_t>(0xAAAA));
}

TEST_CASE("Utf16ScalarKernel encode and decode round-trip scalar boundaries",
          "[core][text][utf16][scalar][edge][roundtrip]")
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

        std::array<char16_t, 2> encoded{};
        const auto encodeResult = Utf16ScalarKernel::encode(*input, encoded);

        CAPTURE(value);

        REQUIRE(encodeResult.status == Utf16Status::Complete);

        UnicodeScalar decoded{};
        const auto decodeResult = Utf16ScalarKernel::decode(
            std::span<const char16_t>{encoded.data(), encodeResult.outputProduced},
            decoded);

        REQUIRE(decodeResult.status == Utf16Status::Complete);
        CHECK(decoded.value() == value);
    }
}

TEST_CASE("Utf16ScalarKernel operations are constexpr", "[core][text][utf16][scalar][constexpr]")
{
    constexpr auto encoded = [] {
        constexpr auto scalar = UnicodeScalar::from(0x1F680);
        static_assert(scalar.has_value());

        std::array<char16_t, 2> output{};
        const auto result = Utf16ScalarKernel::encode(*scalar, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(encoded.second.status == Utf16Status::Complete);
    STATIC_CHECK(encoded.second.inputConsumed == 1);
    STATIC_CHECK(encoded.second.outputProduced == 2);
    STATIC_CHECK(encoded.first[0] == static_cast<char16_t>(0xD83D));
    STATIC_CHECK(encoded.first[1] == static_cast<char16_t>(0xDE80));

    constexpr auto decoded = [] {
        constexpr std::array<char16_t, 2> input{
            static_cast<char16_t>(0xDBFF),
            static_cast<char16_t>(0xDFFF)
        };

        UnicodeScalar scalar{};
        const auto result = Utf16ScalarKernel::decode(input, scalar);

        return std::pair{scalar, result};
    }();

    STATIC_CHECK(decoded.second.status == Utf16Status::Complete);
    STATIC_CHECK(decoded.second.inputConsumed == 2);
    STATIC_CHECK(decoded.second.outputProduced == 1);
    STATIC_CHECK(decoded.first.value() == 0x10FFFF);
}

} // namespace job::core::text::tests