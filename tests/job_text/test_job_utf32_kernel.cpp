#include <array>
#include <cstddef>
#include <span>

#include <catch2/catch_test_macros.hpp>
#include <job_lattice_codec.h>

#include <job_unicode_scalar.h>
#include <job_utf32_kernel.h>
#include <job_utf32_status.h>

namespace job::core::text::tests {
static_assert(lattice::LatticeCodec<Utf32Kernel>);
// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf32Kernel decodes UTF-32 text into Unicode scalars", "[core][text][utf32][kernel][usage]")
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

    const auto result = Utf32Kernel::decode(input, output);

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

TEST_CASE("Utf32Kernel encodes Unicode scalars into UTF-32", "[core][text][utf32][kernel][usage]")
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

    const auto result = Utf32Kernel::encode(input, output);

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

TEST_CASE("Utf32Kernel validates complete UTF-32 text", "[core][text][utf32][kernel][usage]")
{
    constexpr std::array<char32_t, 6> input{
        U'J',
        U'O',
        U'B',
        U' ',
        0x20AC,
        0x1F680
    };

    const auto result = Utf32Kernel::validate(input);

    CHECK(result.status == Utf32Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf32Kernel round-trips mixed Unicode text", "[core][text][utf32][kernel][usage][roundtrip]")
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

    std::array<char32_t, 4> encoded{};

    const auto encodeResult = Utf32Kernel::encode(input, encoded);

    REQUIRE(encodeResult.status == Utf32Status::Complete);
    CHECK(encodeResult.inputConsumed == input.size());

    std::array<UnicodeScalar, 4> decoded{};

    const auto decodeResult = Utf32Kernel::decode(encoded, decoded);

    REQUIRE(decodeResult.status == Utf32Status::Complete);
    REQUIRE(decodeResult.outputProduced == input.size());

    for (std::size_t i = 0; i < input.size(); ++i)
        CHECK(decoded[i].value() == input[i].value());
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("Utf32Kernel handles empty input", "[core][text][utf32][kernel][edge][empty]")
{
    constexpr std::array<char32_t, 0> unitInput{};
    constexpr std::array<UnicodeScalar, 0> scalarInput{};

    std::array<UnicodeScalar, 0> scalarOutput{};
    std::array<char32_t, 0> unitOutput{};

    const auto decodeResult = Utf32Kernel::decode(unitInput, scalarOutput);
    const auto encodeResult = Utf32Kernel::encode(scalarInput, unitOutput);
    const auto validateResult = Utf32Kernel::validate(unitInput);

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

TEST_CASE("Utf32Kernel reports full scalar output without consuming the next unit",
          "[core][text][utf32][kernel][edge][output-full]")
{
    constexpr std::array<char32_t, 2> input{
        U'A',
        0x1F600
    };

    std::array<UnicodeScalar, 1> output{};

    const auto result = Utf32Kernel::decode(input, output);

    REQUIRE(result.status == Utf32Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf32Kernel reports full UTF-32 output without consuming the next scalar",
          "[core][text][utf32][kernel][edge][output-full]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(a.has_value());
    REQUIRE(face.has_value());

    const std::array input{*a, *face};

    std::array<char32_t, 1> output{};

    const auto result = Utf32Kernel::encode(input, output);

    REQUIRE(result.status == Utf32Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0] == U'A');
}

TEST_CASE("Utf32Kernel stops before an invalid surrogate value",
          "[core][text][utf32][kernel][edge][invalid]")
{
    constexpr std::array<char32_t, 3> input{
        U'A',
        U'B',
        0xD800
    };

    std::array<UnicodeScalar, 3> output{};

    const auto result = Utf32Kernel::decode(input, output);

    REQUIRE(result.status == Utf32Status::Invalid);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);
    CHECK(output[0].value() == U'A');
    CHECK(output[1].value() == U'B');
}

TEST_CASE("Utf32Kernel stops before a value above U+10FFFF",
          "[core][text][utf32][kernel][edge][invalid]")
{
    constexpr std::array<char32_t, 2> input{
        U'A',
        0x110000
    };

    std::array<UnicodeScalar, 2> output{};

    const auto result = Utf32Kernel::decode(input, output);

    REQUIRE(result.status == Utf32Status::Invalid);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf32Kernel validation reports the valid prefix before invalid UTF-32",
          "[core][text][utf32][kernel][edge][validate]")
{
    constexpr std::array<char32_t, 4> input{
        U'J',
        0x20AC,
        0x1F680,
        0xDFFF
    };

    const auto result = Utf32Kernel::validate(input);

    CHECK(result.status == Utf32Status::Invalid);
    CHECK(result.inputConsumed == 3);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf32Kernel resumes decoding from returned input progress",
          "[core][text][utf32][kernel][edge][resume]")
{
    constexpr std::array<char32_t, 3> input{
        U'A',
        U'B',
        0x1F680
    };

    std::array<UnicodeScalar, 2> firstOutput{};

    const auto firstResult = Utf32Kernel::decode(input, firstOutput);

    REQUIRE(firstResult.status == Utf32Status::OutputFull);
    REQUIRE(firstResult.inputConsumed == 2);
    REQUIRE(firstResult.outputProduced == 2);

    std::array<UnicodeScalar, 1> secondOutput{};

    const auto secondResult = Utf32Kernel::decode(
        std::span<const char32_t>{input}.subspan(firstResult.inputConsumed),
        secondOutput);

    REQUIRE(secondResult.status == Utf32Status::Complete);
    CHECK(secondResult.inputConsumed == 1);
    CHECK(secondResult.outputProduced == 1);

    CHECK(firstOutput[0].value() == U'A');
    CHECK(firstOutput[1].value() == U'B');
    CHECK(secondOutput[0].value() == 0x1F680);
}

TEST_CASE("Utf32Kernel resumes encoding from returned scalar progress",
          "[core][text][utf32][kernel][edge][resume]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto rocket = UnicodeScalar::from(0x1F680);
    const auto b = UnicodeScalar::from(U'B');

    REQUIRE(a.has_value());
    REQUIRE(rocket.has_value());
    REQUIRE(b.has_value());

    const std::array input{*a, *rocket, *b};

    std::array<char32_t, 2> firstOutput{};

    const auto firstResult = Utf32Kernel::encode(input, firstOutput);

    REQUIRE(firstResult.status == Utf32Status::OutputFull);
    REQUIRE(firstResult.inputConsumed == 2);
    REQUIRE(firstResult.outputProduced == 2);

    CHECK(firstOutput[0] == U'A');
    CHECK(firstOutput[1] == 0x1F680);

    std::array<char32_t, 1> secondOutput{};

    const auto secondResult = Utf32Kernel::encode(
        std::span<const UnicodeScalar>{input}.subspan(firstResult.inputConsumed),
        secondOutput);

    REQUIRE(secondResult.status == Utf32Status::Complete);
    CHECK(secondResult.inputConsumed == 1);
    CHECK(secondResult.outputProduced == 1);
    CHECK(secondOutput[0] == U'B');
}

TEST_CASE("Utf32Kernel operations are constexpr", "[core][text][utf32][kernel][constexpr]")
{
    constexpr auto decoded = [] {
        constexpr std::array<char32_t, 2> input{
            0x1F680,
            U'A'
        };

        std::array<UnicodeScalar, 2> output{};
        const auto result = Utf32Kernel::decode(input, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(decoded.second.status == Utf32Status::Complete);
    STATIC_CHECK(decoded.second.inputConsumed == 2);
    STATIC_CHECK(decoded.second.outputProduced == 2);
    STATIC_CHECK(decoded.first[0].value() == 0x1F680);
    STATIC_CHECK(decoded.first[1].value() == U'A');
}

} // namespace job::core::text::tests