#include <array>
#include <concepts>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include <job_text.h>

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("job_text exposes the Unicode scalar API", "[core][text][usage]")
{
    constexpr auto scalar = UnicodeScalar::from(0x1F680);

    STATIC_REQUIRE(scalar.has_value());
    STATIC_CHECK(scalar->value() == 0x1F680);
}

TEST_CASE("job_text exposes UTF codecs", "[core][text][usage][codec]")
{
    STATIC_CHECK(lattice::LatticeCodec<Utf8>);
    STATIC_CHECK(lattice::LatticeCodec<Utf16>);
    STATIC_CHECK(lattice::LatticeCodec<Utf32>);
}

TEST_CASE("job_text exposes encoding dispatch", "[core][text][usage][dispatch]")
{
    STATIC_CHECK(std::same_as<TextCodec<TextEncoding::Utf8>, Utf8>);
    STATIC_CHECK(std::same_as<TextCodec<TextEncoding::Utf16>, Utf16>);
    STATIC_CHECK(std::same_as<TextCodec<TextEncoding::Utf32>, Utf32>);
}

TEST_CASE("job_text exposes transcoding", "[core][text][usage][transcode]")
{
    constexpr std::array<std::uint8_t, 8> input{
        0x41,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x9A, 0x80
    };

    std::array<char16_t, 4> utf16{};

    const auto toUtf16 = TextTranscoder::transcode<
        TextCodec<TextEncoding::Utf8>,
        TextCodec<TextEncoding::Utf16>
        >(input, utf16);

    REQUIRE(toUtf16.status == TextTranscodeStatus::Complete);
    CHECK(toUtf16.inputConsumed == input.size());
    CHECK(toUtf16.outputProduced == utf16.size());

    std::array<char32_t, 3> utf32{};

    const auto toUtf32 = TextTranscoder::transcode<
        TextCodec<TextEncoding::Utf16>,
        TextCodec<TextEncoding::Utf32>
        >(utf16, utf32);

    REQUIRE(toUtf32.status == TextTranscodeStatus::Complete);
    CHECK(toUtf32.inputConsumed == utf16.size());
    CHECK(toUtf32.outputProduced == utf32.size());

    CHECK(utf32[0] == U'A');
    CHECK(utf32[1] == 0x20AC);
    CHECK(utf32[2] == 0x1F680);
}

TEST_CASE("job_text public surface supports constexpr transcoding", "[core][text][constexpr]")
{
    constexpr auto result = [] {
        constexpr std::array<char32_t, 1> input{
            0x1F680
        };

        std::array<std::uint8_t, 4> output{};

        const auto transcodeResult = TextTranscoder::transcode<
            TextCodec<TextEncoding::Utf32>,
            TextCodec<TextEncoding::Utf8>
            >(input, output);

        return std::pair{output, transcodeResult};
    }();

    STATIC_CHECK(result.second.status == TextTranscodeStatus::Complete);
    STATIC_CHECK(result.second.inputConsumed == 1);
    STATIC_CHECK(result.second.outputProduced == 4);

    STATIC_CHECK(result.first[0] == 0xF0);
    STATIC_CHECK(result.first[1] == 0x9F);
    STATIC_CHECK(result.first[2] == 0x9A);
    STATIC_CHECK(result.first[3] == 0x80);
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("job_text public surface preserves transcode progress", "[core][text][edge][progress]")
{
    constexpr std::array<std::uint8_t, 4> input{
        0x41,
        0x42,
        0xE2,
        0x82
    };

    std::array<char32_t, 4> output{};

    const auto result = TextTranscoder::transcode<
        TextCodec<TextEncoding::Utf8>,
        TextCodec<TextEncoding::Utf32>
        >(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Incomplete);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);

    CHECK(output[0] == U'A');
    CHECK(output[1] == U'B');
}

} // namespace job::core::text::tests