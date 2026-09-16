#include <array>
#include <cstddef>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include <job_lattice_codec.h>
#include <job_unicode_scalar.h>
#include <job_utf32.h>
#include <job_utf32_status.h>

namespace job::core::text::tests {
static_assert(lattice::LatticeCodec<Utf32>);
// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf32 decodes normal UTF-32 text", "[core][text][utf32][usage]")
{
    constexpr std::array<char32_t, 6> input{
        U'J',
        U'O',
        U'B',
        U' ',
        0x20AC,
        0x1F600
    };

    std::array<UnicodeScalar, 6> output{};

    const auto result = Utf32::decode(input, output);

    REQUIRE(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    CHECK(output[0].value() == U'J');
    CHECK(output[1].value() == U'O');
    CHECK(output[2].value() == U'B');
    CHECK(output[3].value() == U' ');
    CHECK(output[4].value() == 0x20AC);
    CHECK(output[5].value() == 0x1F600);
}

TEST_CASE("Utf32 encodes normal Unicode scalar text", "[core][text][utf32][usage]")
{
    const auto j = UnicodeScalar::from(U'J');
    const auto o = UnicodeScalar::from(U'O');
    const auto b = UnicodeScalar::from(U'B');
    const auto euro = UnicodeScalar::from(0x20AC);
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(j.has_value());
    REQUIRE(o.has_value());
    REQUIRE(b.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(face.has_value());

    const std::array input{*j, *o, *b, *euro, *face};

    std::array<char32_t, 5> output{};

    const auto result = Utf32::encode(input, output);

    REQUIRE(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<char32_t, 5> expected{
        U'J',
        U'O',
        U'B',
        0x20AC,
        0x1F600
    };

    CHECK(output == expected);
}

TEST_CASE("Utf32 validates complete UTF-32 text", "[core][text][utf32][usage]")
{
    constexpr std::array<char32_t, 6> input{
        U'J',
        U'O',
        U'B',
        U' ',
        0x20AC,
        0x1F680
    };

    const auto result = Utf32::validate(input);

    CHECK(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf32 supports caller-owned round-trip storage", "[core][text][utf32][usage][roundtrip]")
{
    const auto ascii = UnicodeScalar::from(U'J');
    const auto euro = UnicodeScalar::from(0x20AC);
    const auto rocket = UnicodeScalar::from(0x1F680);

    REQUIRE(ascii.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(rocket.has_value());

    const std::array input{*ascii, *euro, *rocket};

    std::array<char32_t, 3> encoded{};

    const auto encodeResult = Utf32::encode(input, encoded);

    REQUIRE(encodeResult.status == Utf32Status::Complete);
    REQUIRE(encodeResult.inputConsumed == input.size());

    std::array<UnicodeScalar, 3> decoded{};

    const auto decodeResult = Utf32::decode(encoded, decoded);

    REQUIRE(decodeResult.status == Utf32Status::Complete);
    REQUIRE(decodeResult.outputProduced == input.size());

    for (std::size_t i = 0; i < input.size(); ++i)
        CHECK(decoded[i].value() == input[i].value());
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf32 preserves partial progress when decode output is full",
          "[core][text][utf32][edge][output-full]")
{
    constexpr std::array<char32_t, 2> input{
        U'A',
        0x1F600
    };

    std::array<UnicodeScalar, 1> output{};

    const auto result = Utf32::decode(input, output);

    REQUIRE(result.status == Utf32Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf32 preserves partial progress when encode output is full",
          "[core][text][utf32][edge][output-full]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(a.has_value());
    REQUIRE(face.has_value());

    const std::array input{*a, *face};

    std::array<char32_t, 1> output{};

    const auto result = Utf32::encode(input, output);

    REQUIRE(result.status == Utf32Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0] == U'A');
}

TEST_CASE("Utf32 reports invalid scalar values", "[core][text][utf32][edge][invalid]")
{
    constexpr std::array<char32_t, 2> surrogate{
        U'A',
        0xD800
    };

    constexpr std::array<char32_t, 2> tooLarge{
        U'A',
        0x110000
    };

    const auto surrogateResult = Utf32::validate(surrogate);
    const auto tooLargeResult = Utf32::validate(tooLarge);

    CHECK(surrogateResult.status == Utf32Status::Invalid);
    CHECK(surrogateResult.inputConsumed == 1);

    CHECK(tooLargeResult.status == Utf32Status::Invalid);
    CHECK(tooLargeResult.inputConsumed == 1);
}

TEST_CASE("Utf32 handles empty input", "[core][text][utf32][edge][empty]")
{
    constexpr std::array<char32_t, 0> units{};
    constexpr std::array<UnicodeScalar, 0> scalars{};

    std::array<char32_t, 0> unitOutput{};
    std::array<UnicodeScalar, 0> scalarOutput{};

    const auto decodeResult = Utf32::decode(units, scalarOutput);
    const auto encodeResult = Utf32::encode(scalars, unitOutput);
    const auto validateResult = Utf32::validate(units);

    CHECK(decodeResult.status == Utf32Status::Complete);
    CHECK(decodeResult.inputConsumed == 0);
    CHECK(decodeResult.outputProduced == 0);

    CHECK(encodeResult.status == Utf32Status::Complete);
    CHECK(encodeResult.inputConsumed == 0);
    CHECK(encodeResult.outputProduced == 0);

    CHECK(validateResult.status == Utf32Status::Complete);
    CHECK(validateResult.inputConsumed == 0);
    CHECK(validateResult.outputProduced == 0);
}

TEST_CASE("Utf32 facade operations are constexpr", "[core][text][utf32][constexpr]")
{
    constexpr auto encoded = [] {
        constexpr auto rocket = UnicodeScalar::from(0x1F680);
        static_assert(rocket.has_value());

        constexpr std::array input{*rocket};

        std::array<char32_t, 1> output{};
        const auto result = Utf32::encode(input, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(encoded.second.status == Utf32Status::Complete);
    STATIC_CHECK(encoded.second.inputConsumed == 1);
    STATIC_CHECK(encoded.second.outputProduced == 1);
    STATIC_CHECK(encoded.first[0] == 0x1F680);

    constexpr auto valid = [] {
        constexpr std::array<char32_t, 1> input{0x10FFFF};
        return Utf32::validate(input);
    }();

    STATIC_CHECK(valid.status == Utf32Status::Complete);
    STATIC_CHECK(valid.inputConsumed == 1);
}

} // namespace job::core::text::tests