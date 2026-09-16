#include <array>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include <job_lattice_codec.h>

#include <job_unicode_scalar.h>
#include <job_utf8_kernel.h>
#include <job_utf8_status.h>

namespace job::core::text::tests {
static_assert(lattice::LatticeCodec<Utf8Kernel>);
// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("Utf8Kernel decodes UTF-8 text into Unicode scalars", "[core][text][utf8][kernel][usage]")
{
    constexpr std::array<std::uint8_t, 11> input{
        0x4A,                         // J
        0x4F,                         // O
        0x42,                         // B
        0x20,                         // space
        0xE2, 0x82, 0xAC,             // €
        0xF0, 0x9F, 0x98, 0x80        // 😀
    };

    std::array<UnicodeScalar, 6> output{};

    const auto result = Utf8Kernel::decode(input, output);

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

TEST_CASE("Utf8Kernel encodes Unicode scalars into UTF-8", "[core][text][utf8][kernel][usage]")
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

    std::array<std::uint8_t, 10> output{};

    const auto result = Utf8Kernel::encode(input, output);

    REQUIRE(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<std::uint8_t, 10> expected{
        0x4A,
        0x4F,
        0x42,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x98, 0x80
    };

    CHECK(output == expected);
}

TEST_CASE("Utf8Kernel validates complete UTF-8 text", "[core][text][utf8][kernel][usage]")
{
    constexpr std::array<std::uint8_t, 12> input{
        0x4A, 0x4F, 0x42, 0x20,
        0xE2, 0x82, 0xAC,
        0x20,
        0xF0, 0x9F, 0x98, 0x80
    };

    const auto result = Utf8Kernel::validate(input);

    CHECK(result.status == Utf8Status::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8Kernel round-trips mixed Unicode text", "[core][text][utf8][kernel][usage]")
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

    std::array<std::uint8_t, 15> encoded{};

    const auto encodeResult = Utf8Kernel::encode(input, encoded);

    REQUIRE(encodeResult.status == Utf8Status::Complete);
    CHECK(encodeResult.inputConsumed == input.size());

    std::array<UnicodeScalar, 4> decoded{};

    const auto decodeResult = Utf8Kernel::decode(
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

TEST_CASE("Utf8Kernel handles empty input", "[core][text][utf8][kernel][edge]")
{
    constexpr std::array<std::uint8_t, 0> byteInput{};
    constexpr std::array<UnicodeScalar, 0> scalarInput{};

    std::array<UnicodeScalar, 0> scalarOutput{};
    std::array<std::uint8_t, 0> byteOutput{};

    const auto decodeResult = Utf8Kernel::decode(byteInput, scalarOutput);
    const auto encodeResult = Utf8Kernel::encode(scalarInput, byteOutput);
    const auto validateResult = Utf8Kernel::validate(byteInput);

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

TEST_CASE("Utf8Kernel reports full scalar output without consuming the next scalar",
          "[core][text][utf8][kernel][edge][output-full]")
{
    constexpr std::array<std::uint8_t, 5> input{
        0x41,                         // A
        0xF0, 0x9F, 0x98, 0x80        // 😀
    };

    std::array<UnicodeScalar, 1> output{};

    const auto result = Utf8Kernel::decode(input, output);

    REQUIRE(result.status == Utf8Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf8Kernel reports full byte output without consuming the next scalar",
          "[core][text][utf8][kernel][edge][output-full]")
{
    const auto ascii = UnicodeScalar::from(U'A');
    const auto face = UnicodeScalar::from(0x1F600);

    REQUIRE(ascii.has_value());
    REQUIRE(face.has_value());

    const std::array input{*ascii, *face};

    std::array<std::uint8_t, 4> output{};

    const auto result = Utf8Kernel::encode(input, output);

    REQUIRE(result.status == Utf8Status::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0] == 0x41);
}

TEST_CASE("Utf8Kernel stops before an incomplete trailing sequence",
          "[core][text][utf8][kernel][edge][incomplete]")
{
    constexpr std::array<std::uint8_t, 4> input{
        0x41,
        0xE2, 0x82,
        0x00
    };

    std::array<UnicodeScalar, 4> output{};

    const auto result = Utf8Kernel::decode(
        std::span<const std::uint8_t>{input.data(), 3},
        output);

    REQUIRE(result.status == Utf8Status::Incomplete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0].value() == U'A');
}

TEST_CASE("Utf8Kernel stops before an invalid sequence",
          "[core][text][utf8][kernel][edge][invalid]")
{
    constexpr std::array<std::uint8_t, 5> input{
        0x41,
        0x42,
        0xE2, 0x28, 0xA1
    };

    std::array<UnicodeScalar, 5> output{};

    const auto result = Utf8Kernel::decode(input, output);

    REQUIRE(result.status == Utf8Status::Invalid);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);
    CHECK(output[0].value() == U'A');
    CHECK(output[1].value() == U'B');
}

TEST_CASE("Utf8Kernel validation reports the valid prefix before invalid UTF-8",
          "[core][text][utf8][kernel][edge][validate]")
{
    constexpr std::array<std::uint8_t, 6> input{
        0x4A,
        0xE2, 0x82, 0xAC,
        0xC0, 0xAF
    };

    const auto result = Utf8Kernel::validate(input);

    CHECK(result.status == Utf8Status::Invalid);
    CHECK(result.inputConsumed == 4);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8Kernel validation reports incomplete trailing UTF-8",
          "[core][text][utf8][kernel][edge][validate]")
{
    constexpr std::array<std::uint8_t, 3> input{
        0x41,
        0xF0,
        0x9F
    };

    const auto result = Utf8Kernel::validate(input);

    CHECK(result.status == Utf8Status::Incomplete);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("Utf8Kernel resumes decoding from returned input progress",
          "[core][text][utf8][kernel][edge][resume]")
{
    constexpr std::array<std::uint8_t, 6> input{
        0x41,
        0x42,
        0xE2, 0x82, 0xAC,
        0x43
    };

    std::array<UnicodeScalar, 2> firstOutput{};

    const auto firstResult = Utf8Kernel::decode(input, firstOutput);

    REQUIRE(firstResult.status == Utf8Status::OutputFull);
    REQUIRE(firstResult.inputConsumed == 2);
    REQUIRE(firstResult.outputProduced == 2);

    std::array<UnicodeScalar, 2> secondOutput{};

    const auto secondResult = Utf8Kernel::decode(
        std::span<const std::uint8_t>{input}.subspan(firstResult.inputConsumed),
        secondOutput);

    REQUIRE(secondResult.status == Utf8Status::Complete);
    CHECK(secondResult.inputConsumed == 4);
    CHECK(secondResult.outputProduced == 2);

    CHECK(firstOutput[0].value() == U'A');
    CHECK(firstOutput[1].value() == U'B');
    CHECK(secondOutput[0].value() == 0x20AC);
    CHECK(secondOutput[1].value() == U'C');
}

TEST_CASE("Utf8Kernel resumes encoding from returned scalar progress",
          "[core][text][utf8][kernel][edge][resume]")
{
    const auto a = UnicodeScalar::from(U'A');
    const auto euro = UnicodeScalar::from(0x20AC);
    const auto b = UnicodeScalar::from(U'B');

    REQUIRE(a.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(b.has_value());

    const std::array input{*a, *euro, *b};

    std::array<std::uint8_t, 2> firstOutput{};

    const auto firstResult = Utf8Kernel::encode(input, firstOutput);

    REQUIRE(firstResult.status == Utf8Status::OutputFull);
    REQUIRE(firstResult.inputConsumed == 1);
    REQUIRE(firstResult.outputProduced == 1);
    CHECK(firstOutput[0] == 0x41);

    std::array<std::uint8_t, 4> secondOutput{};

    const auto secondResult = Utf8Kernel::encode(
        std::span<const UnicodeScalar>{input}.subspan(firstResult.inputConsumed),
        secondOutput);

    REQUIRE(secondResult.status == Utf8Status::Complete);
    CHECK(secondResult.inputConsumed == 2);
    CHECK(secondResult.outputProduced == 4);

    CHECK(secondOutput[0] == 0xE2);
    CHECK(secondOutput[1] == 0x82);
    CHECK(secondOutput[2] == 0xAC);
    CHECK(secondOutput[3] == 0x42);
}

TEST_CASE("Utf8Kernel operations are constexpr", "[core][text][utf8][kernel][constexpr]")
{
    constexpr auto decoded = [] {
        constexpr std::array<std::uint8_t, 4> input{
            0xE2, 0x82, 0xAC,
            0x41
        };

        std::array<UnicodeScalar, 2> output{};
        const auto result = Utf8Kernel::decode(input, output);

        return std::pair{output, result};
    }();

    STATIC_CHECK(decoded.second.status == Utf8Status::Complete);
    STATIC_CHECK(decoded.second.inputConsumed == 4);
    STATIC_CHECK(decoded.second.outputProduced == 2);
    STATIC_CHECK(decoded.first[0].value() == 0x20AC);
    STATIC_CHECK(decoded.first[1].value() == U'A');
}

} // namespace job::core::text::tests