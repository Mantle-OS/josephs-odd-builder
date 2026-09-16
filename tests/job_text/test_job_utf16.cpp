#include <array>
#include <cstddef>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include <job_lattice_codec.h>
#include <job_unicode_scalar.h>
#include <job_utf16.h>
#include <job_utf16_status.h>

namespace job::core::text::tests {
static_assert(lattice::LatticeCodec<Utf16>);
// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf16 decodes normal UTF-16 text", "[core][text][utf16][usage]")
{
    constexpr std::array<char16_t, 7> input{
        u'J',
        u'O',
        u'B',
        u' ',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE00)
    };

    std::array<UnicodeScalar, 6> output{};

    const auto result = Utf16::decode(input, output);

    REQUIRE(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    CHECK(output[0].value() == U'J');
    CHECK(output[1].value() == U'O');
    CHECK(output[2].value() == U'B');
    CHECK(output[3].value() == U' ');
    CHECK(output[4].value() == 0x20AC);
    CHECK(output[5].value() == 0x1F600);
}

TEST_CASE("Utf16 encodes normal Unicode scalar text", "[core][text][utf16][usage]")
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

    std::array<char16_t, 6> output{};

    const auto result = Utf16::encode(input, output);

    REQUIRE(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<char16_t, 6> expected{
        u'J',
        u'O',
        u'B',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE00)
    };

    CHECK(output == expected);
}

TEST_CASE("Utf16 validates complete UTF-16 text", "[core][text][utf16][usage]")
{
    constexpr std::array<char16_t, 7> input{
        u'J',
        u'O',
        u'B',
        u' ',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE80)
    };

    const auto result = Utf16::validate(input);

    CHECK(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16 supports caller-owned round-trip storage", "[core][text][utf16][usage][roundtrip]")
{
    const auto ascii = UnicodeScalar::from(U'J');
    const auto euro = UnicodeScalar::from(0x20AC);
    const auto rocket = UnicodeScalar::from(0x1F680);

    REQUIRE(ascii.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(rocket.has_value());

    const std::array input{*ascii, *euro, *rocket};

    std::array<char16_t, 4> encoded{};

    const auto encodeResult = Utf16::encode(input, encoded);

    REQUIRE(encodeResult.status == Utf16Status::Complete);
    REQUIRE(encodeResult.inputConsumed == input.size());

    std::array<UnicodeScalar, 3> decoded{};

    const auto decodeResult = Utf16::decode(
        std::span<const char16_t>{encoded.data(), encodeResult.outputProduced},
        decoded);

    REQUIRE(decodeResult.status == Utf16Status::Complete);
    REQUIRE(decodeResult.outputProduced == input.size());

    for (std::size_t i = 0; i < input.size(); ++i)
        CHECK(decoded[i].value() == input[i].value());
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf16 preserves partial progress when decode output is full",
          "[core][text][utf16][edge][output-full]")
{
    constexpr std::array<char16_t, 3> input{
        u'A',
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE00)
    };

    std::array<UnicodeScalar, 1> output{};

    const auto result = Utf16::decode(input, output);

    REQUIRE(result.status == Utf16Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf16 preserves partial progress when encode output is full",
          "[core][text][utf16][edge][output-full]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(a.has_value());
    REQUIRE(face.has_value());

    const std::array input{*a, *face};

    std::array<char16_t, 2> output{
        static_cast<char16_t>(0xAAAA),
        static_cast<char16_t>(0xBBBB)
    };

    const auto result = Utf16::encode(input, output);

    REQUIRE(result.status == Utf16Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);

    CHECK(output[0] == u'A');
    CHECK(output[1] == static_cast<char16_t>(0xBBBB));
}

TEST_CASE("Utf16 distinguishes incomplete input from invalid input",
          "[core][text][utf16][edge][status]")
{
    constexpr std::array<char16_t, 1> incomplete{
        static_cast<char16_t>(0xD83D)
    };

    constexpr std::array<char16_t, 2> invalid{
        static_cast<char16_t>(0xD83D),
        u'A'
    };

    const auto incompleteResult = Utf16::validate(incomplete);
    const auto invalidResult = Utf16::validate(invalid);

    CHECK(incompleteResult.status == Utf16Status::Incomplete);
    CHECK(incompleteResult.inputConsumed == 0);

    CHECK(invalidResult.status == Utf16Status::Invalid);
    CHECK(invalidResult.inputConsumed == 0);
}

TEST_CASE("Utf16 handles empty input", "[core][text][utf16][edge][empty]")
{
    constexpr std::array<char16_t, 0> units{};
    constexpr std::array<UnicodeScalar, 0> scalars{};

    std::array<char16_t, 0> unitOutput{};
    std::array<UnicodeScalar, 0> scalarOutput{};

    const auto decodeResult = Utf16::decode(units, scalarOutput);
    const auto encodeResult = Utf16::encode(scalars, unitOutput);
    const auto validateResult = Utf16::validate(units);

    CHECK(decodeResult.status == Utf16Status::Complete);
    CHECK(decodeResult.inputConsumed == 0);
    CHECK(decodeResult.outputProduced == 0);

    CHECK(encodeResult.status == Utf16Status::Complete);
    CHECK(encodeResult.inputConsumed == 0);
    CHECK(encodeResult.outputProduced == 0);

    CHECK(validateResult.status == Utf16Status::Complete);
    CHECK(validateResult.inputConsumed == 0);
    CHECK(validateResult.outputProduced == 0);
}

TEST_CASE("Utf16 facade operations are constexpr", "[core][text][utf16][constexpr]")
{
    constexpr auto encoded = [] {
        constexpr auto rocket = UnicodeScalar::from(0x1F680);
        static_assert(rocket.has_value());

        constexpr std::array input{*rocket};

        std::array<char16_t, 2> output{};
        const auto result = Utf16::encode(input, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(encoded.second.status == Utf16Status::Complete);
    STATIC_CHECK(encoded.second.inputConsumed == 1);
    STATIC_CHECK(encoded.second.outputProduced == 2);

    STATIC_CHECK(encoded.first[0] == static_cast<char16_t>(0xD83D));
    STATIC_CHECK(encoded.first[1] == static_cast<char16_t>(0xDE80));

    constexpr auto valid = [] {
        constexpr std::array<char16_t, 2> input{
            static_cast<char16_t>(0xD83D),
            static_cast<char16_t>(0xDE80)
        };

        return Utf16::validate(input);
    }();

    STATIC_CHECK(valid.status == Utf16Status::Complete);
    STATIC_CHECK(valid.inputConsumed == 2);
}

} // namespace job::core::text::tests