#include <array>
#include <cstddef>
#include <span>

#include <catch2/catch_test_macros.hpp>
#include <job_lattice_codec.h>

#include <job_unicode_scalar.h>
#include <job_utf16_kernel.h>
#include <job_utf16_status.h>

namespace job::core::text::tests {
static_assert(lattice::LatticeCodec<Utf16Kernel>);
// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf16Kernel decodes UTF-16 text into Unicode scalars", "[core][text][utf16][kernel][usage]")
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

    const auto result = Utf16Kernel::decode(input, output);

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

TEST_CASE("Utf16Kernel encodes Unicode scalars into UTF-16", "[core][text][utf16][kernel][usage]")
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

    const std::array input{
        *j,
        *o,
        *b,
        *euro,
        *face
    };

    std::array<char16_t, 6> output{};

    const auto result = Utf16Kernel::encode(input, output);

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

TEST_CASE("Utf16Kernel validates complete UTF-16 text", "[core][text][utf16][kernel][usage]")
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

    const auto result = Utf16Kernel::validate(input);

    CHECK(result.status == Utf16Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16Kernel round-trips mixed Unicode text", "[core][text][utf16][kernel][usage][roundtrip]")
{
    const auto ascii = UnicodeScalar::from(U'J');
    const auto euro = UnicodeScalar::from(0x20AC);
    const auto rocket = UnicodeScalar::from(0x1F680);
    const auto maximum = UnicodeScalar::from(0x10FFFF);

    REQUIRE(ascii.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(rocket.has_value());
    REQUIRE(maximum.has_value());

    const std::array input{
        *ascii,
        *euro,
        *rocket,
        *maximum
    };

    std::array<char16_t, 6> encoded{};

    const auto encodeResult = Utf16Kernel::encode(input, encoded);

    REQUIRE(encodeResult.status == Utf16Status::Complete);
    CHECK(encodeResult.inputConsumed == input.size());

    std::array<UnicodeScalar, 4> decoded{};

    const auto decodeResult = Utf16Kernel::decode(
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

TEST_CASE("Utf16Kernel handles empty input", "[core][text][utf16][kernel][edge][empty]")
{
    constexpr std::array<char16_t, 0> unitInput{};
    constexpr std::array<UnicodeScalar, 0> scalarInput{};

    std::array<UnicodeScalar, 0> scalarOutput{};
    std::array<char16_t, 0> unitOutput{};

    const auto decodeResult = Utf16Kernel::decode(unitInput, scalarOutput);
    const auto encodeResult = Utf16Kernel::encode(scalarInput, unitOutput);
    const auto validateResult = Utf16Kernel::validate(unitInput);

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

TEST_CASE("Utf16Kernel reports full scalar output without consuming the next scalar",
          "[core][text][utf16][kernel][edge][output-full]")
{
    constexpr std::array<char16_t, 3> input{
        u'A',
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE00)
    };

    std::array<UnicodeScalar, 1> output{};

    const auto result = Utf16Kernel::decode(input, output);

    REQUIRE(result.status == Utf16Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf16Kernel does not emit half a surrogate pair when output is full",
          "[core][text][utf16][kernel][edge][output-full]")
{
    const auto ascii = UnicodeScalar::from(U'A');
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(ascii.has_value());
    REQUIRE(face.has_value());

    const std::array input{*ascii, *face};

    std::array<char16_t, 2> output{
        static_cast<char16_t>(0xAAAA),
        static_cast<char16_t>(0xBBBB)
    };

    const auto result = Utf16Kernel::encode(input, output);

    REQUIRE(result.status == Utf16Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);

    CHECK(output[0] == u'A');
    CHECK(output[1] == static_cast<char16_t>(0xBBBB));
}

TEST_CASE("Utf16Kernel stops before an incomplete trailing surrogate pair",
          "[core][text][utf16][kernel][edge][incomplete]")
{
    constexpr std::array<char16_t, 3> input{
        u'A',
        u'B',
        static_cast<char16_t>(0xD83D)
    };

    std::array<UnicodeScalar, 3> output{};

    const auto result = Utf16Kernel::decode(input, output);

    REQUIRE(result.status == Utf16Status::Incomplete);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);
    CHECK(output[0].value() == U'A');
    CHECK(output[1].value() == U'B');
}

TEST_CASE("Utf16Kernel stops before an invalid surrogate sequence",
          "[core][text][utf16][kernel][edge][invalid]")
{
    constexpr std::array<char16_t, 4> input{
        u'A',
        u'B',
        static_cast<char16_t>(0xD83D),
        u'C'
    };

    std::array<UnicodeScalar, 4> output{};

    const auto result = Utf16Kernel::decode(input, output);

    REQUIRE(result.status == Utf16Status::Invalid);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);
    CHECK(output[0].value() == U'A');
    CHECK(output[1].value() == U'B');
}

TEST_CASE("Utf16Kernel stops before an isolated low surrogate",
          "[core][text][utf16][kernel][edge][invalid]")
{
    constexpr std::array<char16_t, 3> input{
        u'A',
        static_cast<char16_t>(0xDC00),
        u'B'
    };

    std::array<UnicodeScalar, 3> output{};

    const auto result = Utf16Kernel::decode(input, output);

    REQUIRE(result.status == Utf16Status::Invalid);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf16Kernel validation reports the valid prefix before invalid UTF-16",
          "[core][text][utf16][kernel][edge][validate]")
{
    constexpr std::array<char16_t, 4> input{
        u'J',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        u'X'
    };

    const auto result = Utf16Kernel::validate(input);

    CHECK(result.status == Utf16Status::Invalid);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16Kernel validation reports incomplete trailing UTF-16",
          "[core][text][utf16][kernel][edge][validate]")
{
    constexpr std::array<char16_t, 2> input{
        u'A',
        static_cast<char16_t>(0xD83D)
    };

    const auto result = Utf16Kernel::validate(input);

    CHECK(result.status == Utf16Status::Incomplete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf16Kernel resumes decoding from returned input progress",
          "[core][text][utf16][kernel][edge][resume]")
{
    constexpr std::array<char16_t, 4> input{
        u'A',
        u'B',
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE80)
    };

    std::array<UnicodeScalar, 2> firstOutput{};

    const auto firstResult = Utf16Kernel::decode(input, firstOutput);

    REQUIRE(firstResult.status == Utf16Status::OutputFull);
    REQUIRE(firstResult.inputConsumed == 2);
    REQUIRE(firstResult.outputProduced == 2);

    std::array<UnicodeScalar, 1> secondOutput{};

    const auto secondResult = Utf16Kernel::decode(
        std::span<const char16_t>{input}.subspan(firstResult.inputConsumed),
        secondOutput);

    REQUIRE(secondResult.status == Utf16Status::Complete);
    CHECK(secondResult.inputConsumed == 2);
    CHECK(secondResult.outputProduced == 1);

    CHECK(firstOutput[0].value() == U'A');
    CHECK(firstOutput[1].value() == U'B');
    CHECK(secondOutput[0].value() == 0x1F680);
}

TEST_CASE("Utf16Kernel resumes encoding from returned scalar progress",
          "[core][text][utf16][kernel][edge][resume]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto rocket = UnicodeScalar::from(0x1F680);
    const auto b = UnicodeScalar::from(U'B');

    REQUIRE(a.has_value());
    REQUIRE(rocket.has_value());
    REQUIRE(b.has_value());

    const std::array input{*a, *rocket, *b};

    std::array<char16_t, 2> firstOutput {
        static_cast<char16_t>(0xAAAA),
        static_cast<char16_t>(0xBBBB)
    };

    const auto firstResult = Utf16Kernel::encode(input, firstOutput);

    REQUIRE(firstResult.status == Utf16Status::OutputFull);
    REQUIRE(firstResult.inputConsumed == 1);
    REQUIRE(firstResult.outputProduced == 1);
    CHECK(firstOutput[0] == u'A');
    CHECK(firstOutput[1] == static_cast<char16_t>(0xBBBB));

    std::array<char16_t, 3> secondOutput{};

    const auto secondResult = Utf16Kernel::encode(
        std::span<const UnicodeScalar>{input}.subspan(firstResult.inputConsumed),
        secondOutput);

    REQUIRE(secondResult.status == Utf16Status::Complete);
    CHECK(secondResult.inputConsumed == 2);
    CHECK(secondResult.outputProduced == 3);

    CHECK(secondOutput[0] == static_cast<char16_t>(0xD83D));
    CHECK(secondOutput[1] == static_cast<char16_t>(0xDE80));
    CHECK(secondOutput[2] == u'B');
}

TEST_CASE("Utf16Kernel operations are constexpr", "[core][text][utf16][kernel][constexpr]")
{
    constexpr auto decoded = [] {
        constexpr std::array<char16_t, 3> input {
            static_cast<char16_t>(0xD83D),
            static_cast<char16_t>(0xDE80),
            u'A'
        };

        std::array<UnicodeScalar, 2> output{};
        const auto result = Utf16Kernel::decode(input, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(decoded.second.status == Utf16Status::Complete);
    STATIC_CHECK(decoded.second.inputConsumed == 3);
    STATIC_CHECK(decoded.second.outputProduced == 2);
    STATIC_CHECK(decoded.first[0].value() == 0x1F680);
    STATIC_CHECK(decoded.first[1].value() == U'A');
}

} // namespace job::core::text::tests