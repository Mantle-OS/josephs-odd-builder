#include <array>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>
#include <job_lattice_codec.h>
#include <job_unicode_scalar.h>
#include <job_utf8.h>
#include <job_utf8_status.h>

namespace job::core::text::tests {
static_assert(lattice::LatticeCodec<Utf8>);
// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf8 decodes normal UTF-8 text", "[core][text][utf8][usage]")
{
    constexpr std::array<std::uint8_t, 11> input{
        0x4A, 0x4F, 0x42, 0x20,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x98, 0x80
    };

    std::array<UnicodeScalar, 6> output{};

    const auto result = Utf8::decode(input, output);

    REQUIRE(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    CHECK(output[0].value() == U'J');
    CHECK(output[1].value() == U'O');
    CHECK(output[2].value() == U'B');
    CHECK(output[3].value() == U' ');
    CHECK(output[4].value() == 0x20AC);
    CHECK(output[5].value() == 0x1F600);
}

TEST_CASE("Utf8 encodes normal Unicode scalar text", "[core][text][utf8][usage]")
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

    std::array<std::uint8_t, 10> output{};

    const auto result = Utf8::encode(input, output);

    REQUIRE(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<std::uint8_t, 10> expected{
        0x4A, 0x4F, 0x42,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x98, 0x80
    };

    CHECK(output == expected);
}

TEST_CASE("Utf8 validates complete text", "[core][text][utf8][usage]")
{
    constexpr std::array<std::uint8_t, 12> input{
        0x4A, 0x4F, 0x42, 0x20,
        0xE2, 0x82, 0xAC, 0x20,
        0xF0, 0x9F, 0x98, 0x80
    };

    const auto result = Utf8::validate(input);

    CHECK(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8 supports caller-owned round-trip storage", "[core][text][utf8][usage][roundtrip]")
{
    const auto j = UnicodeScalar::from(U'J');
    const auto euro = UnicodeScalar::from(0x20AC);
    const auto rocket = UnicodeScalar::from(0x1F680);

    REQUIRE(j.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(rocket.has_value());

    const std::array input{*j, *euro, *rocket};

    std::array<std::uint8_t, 8> encoded{};

    const auto encodeResult = Utf8::encode(input, encoded);

    REQUIRE(encodeResult.status == Utf8Status::Complete);
    REQUIRE(encodeResult.inputConsumed == input.size());

    std::array<UnicodeScalar, 3> decoded{};

    const auto decodeResult = Utf8::decode(
        std::span<const std::uint8_t>{encoded.data(), encodeResult.outputProduced},
        decoded);

    REQUIRE(decodeResult.status == Utf8Status::Complete);
    REQUIRE(decodeResult.outputProduced == input.size());

    for (std::size_t i = 0; i < input.size(); ++i)
        CHECK(decoded[i].value() == input[i].value());
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf8 preserves partial progress when decode output is full",
          "[core][text][utf8][edge][output-full]")
{
    constexpr std::array<std::uint8_t, 5> input{
        0x41,
        0xF0, 0x9F, 0x98, 0x80
    };

    std::array<UnicodeScalar, 1> output{};

    const auto result = Utf8::decode(input, output);

    REQUIRE(result.status == Utf8Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf8 preserves partial progress when encode output is full",
          "[core][text][utf8][edge][output-full]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(a.has_value());
    REQUIRE(face.has_value());

    const std::array input{*a, *face};

    std::array<std::uint8_t, 3> output{};

    const auto result = Utf8::encode(input, output);

    REQUIRE(result.status == Utf8Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0] == 0x41);
}

TEST_CASE("Utf8 distinguishes incomplete input from invalid input",
          "[core][text][utf8][edge][status]")
{
    constexpr std::array<std::uint8_t, 2> incomplete{0xE2, 0x82};
    constexpr std::array<std::uint8_t, 3> invalid{0xE2, 0x28, 0xA1};

    const auto incompleteResult = Utf8::validate(incomplete);
    const auto invalidResult = Utf8::validate(invalid);

    CHECK(incompleteResult.status == Utf8Status::Incomplete);
    CHECK(incompleteResult.inputConsumed == 0);

    CHECK(invalidResult.status == Utf8Status::Invalid);
    CHECK(invalidResult.inputConsumed == 0);
}

TEST_CASE("Utf8 handles empty input", "[core][text][utf8][edge][empty]")
{
    constexpr std::array<std::uint8_t, 0> bytes{};
    constexpr std::array<UnicodeScalar, 0> scalars{};

    std::array<std::uint8_t, 0> byteOutput{};
    std::array<UnicodeScalar, 0> scalarOutput{};

    const auto decodeResult = Utf8::decode(bytes, scalarOutput);
    const auto encodeResult = Utf8::encode(scalars, byteOutput);
    const auto validateResult = Utf8::validate(bytes);

    CHECK(decodeResult.status == Utf8Status::Complete);
    CHECK(decodeResult.inputConsumed == 0);
    CHECK(decodeResult.outputProduced == 0);

    CHECK(encodeResult.status == Utf8Status::Complete);
    CHECK(encodeResult.inputConsumed == 0);
    CHECK(encodeResult.outputProduced == 0);

    CHECK(validateResult.status == Utf8Status::Complete);
    CHECK(validateResult.inputConsumed == 0);
    CHECK(validateResult.outputProduced == 0);
}

TEST_CASE("Utf8 facade operations are constexpr", "[core][text][utf8][constexpr]")
{
    constexpr auto encoded = [] {
        constexpr auto euro = UnicodeScalar::from(0x20AC);
        static_assert(euro.has_value());

        constexpr std::array input{*euro};

        std::array<std::uint8_t, 3> output{};
        const auto result = Utf8::encode(input, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(encoded.second.status == Utf8Status::Complete);
    STATIC_CHECK(encoded.second.inputConsumed == 1);
    STATIC_CHECK(encoded.second.outputProduced == 3);

    STATIC_CHECK(encoded.first[0] == 0xE2);
    STATIC_CHECK(encoded.first[1] == 0x82);
    STATIC_CHECK(encoded.first[2] == 0xAC);

    constexpr auto valid = [] {
        constexpr std::array<std::uint8_t, 4> input{0xF0, 0x9F, 0x9A, 0x80};
        return Utf8::validate(input);
    }();

    STATIC_CHECK(valid.status == Utf8Status::Complete);
    STATIC_CHECK(valid.inputConsumed == 4);
}

} // namespace job::core::text::tests