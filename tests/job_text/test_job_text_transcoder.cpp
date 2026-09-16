#include <array>
#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include <job_text_transcoder.h>
#include <job_text_transcode_status.h>

#include <job_utf8.h>
#include <job_utf16.h>
#include <job_utf32.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("TextTranscoder converts UTF-8 to UTF-16", "[core][text][transcode][utf8][utf16][usage]")
{
    constexpr std::array<std::uint8_t, 11> input{
        0x4A, 0x4F, 0x42, 0x20,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x98, 0x80
    };

    std::array<char16_t, 7> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf16>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<char16_t, 7> expected{
        u'J',
        u'O',
        u'B',
        u' ',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE00)
    };

    CHECK(output == expected);
}

TEST_CASE("TextTranscoder converts UTF-16 to UTF-8", "[core][text][transcode][utf16][utf8][usage]")
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

    std::array<std::uint8_t, 11> output{};

    const auto result = TextTranscoder::transcode<Utf16, Utf8>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<std::uint8_t, 11> expected{
        0x4A, 0x4F, 0x42, 0x20,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x98, 0x80
    };

    CHECK(output == expected);
}

TEST_CASE("TextTranscoder converts UTF-8 to UTF-32", "[core][text][transcode][utf8][utf32][usage]")
{
    constexpr std::array<std::uint8_t, 8> input{
        0x41,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x9A, 0x80
    };

    std::array<char32_t, 3> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf32>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    CHECK(output[0] == U'A');
    CHECK(output[1] == 0x20AC);
    CHECK(output[2] == 0x1F680);
}

TEST_CASE("TextTranscoder converts UTF-32 to UTF-8", "[core][text][transcode][utf32][utf8][usage]")
{
    constexpr std::array<char32_t, 3> input{
        U'A',
        0x20AC,
        0x1F680
    };

    std::array<std::uint8_t, 8> output{};

    const auto result = TextTranscoder::transcode<Utf32, Utf8>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<std::uint8_t, 8> expected{
        0x41,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x9A, 0x80
    };

    CHECK(output == expected);
}

TEST_CASE("TextTranscoder converts UTF-16 to UTF-32", "[core][text][transcode][utf16][utf32][usage]")
{
    constexpr std::array<char16_t, 4> input{
        u'A',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE80)
    };

    std::array<char32_t, 3> output{};

    const auto result = TextTranscoder::transcode<Utf16, Utf32>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    CHECK(output[0] == U'A');
    CHECK(output[1] == 0x20AC);
    CHECK(output[2] == 0x1F680);
}

TEST_CASE("TextTranscoder converts UTF-32 to UTF-16", "[core][text][transcode][utf32][utf16][usage]")
{
    constexpr std::array<char32_t, 3> input{
        U'A',
        0x20AC,
        0x1F680
    };

    std::array<char16_t, 4> output{};

    const auto result = TextTranscoder::transcode<Utf32, Utf16>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    constexpr std::array<char16_t, 4> expected{
        u'A',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE80)
    };

    CHECK(output == expected);
}

TEST_CASE("TextTranscoder preserves Unicode scalar values across every UTF representation",
          "[core][text][transcode][usage][roundtrip]")
{
    constexpr std::array fixtures{
        TextTranscodeScalarFixture{0x000000},
        TextTranscodeScalarFixture{0x00007F},
        TextTranscodeScalarFixture{0x000080},
        TextTranscodeScalarFixture{0x00D7FF},
        TextTranscodeScalarFixture{0x00E000},
        TextTranscodeScalarFixture{0x00FFFF},
        TextTranscodeScalarFixture{0x010000},
        TextTranscodeScalarFixture{0x1F680},
        TextTranscodeScalarFixture{0x10FFFF}
    };

    for (const auto &fixture : fixtures) {
        const std::array<char32_t, 1> utf32Input{fixture.scalar};

        std::array<std::uint8_t, 4> utf8{};
        const auto toUtf8 = TextTranscoder::transcode<Utf32, Utf8>(utf32Input, utf8);

        CAPTURE(fixture.scalar);

        REQUIRE(toUtf8.status == TextTranscodeStatus::Complete);

        std::array<char16_t, 2> utf16{};
        const auto toUtf16 = TextTranscoder::transcode<Utf8, Utf16>(
            std::span<const std::uint8_t>{utf8.data(), toUtf8.outputProduced},
            utf16);

        REQUIRE(toUtf16.status == TextTranscodeStatus::Complete);

        std::array<char32_t, 1> utf32Output{};
        const auto backToUtf32 = TextTranscoder::transcode<Utf16, Utf32>(
            std::span<const char16_t>{utf16.data(), toUtf16.outputProduced},
            utf32Output);

        REQUIRE(backToUtf32.status == TextTranscodeStatus::Complete);
        CHECK(utf32Output[0] == fixture.scalar);
    }
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("TextTranscoder handles empty input", "[core][text][transcode][edge][empty]")
{
    constexpr std::array<std::uint8_t, 0> input{};
    std::array<char16_t, 0> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf16>(input, output);

    CHECK(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == 0);
    CHECK(result.outputProduced == 0);
}

TEST_CASE("TextTranscoder reports incomplete source input", "[core][text][transcode][edge][incomplete]")
{
    constexpr std::array<std::uint8_t, 4> input{
        0x41,
        0x42,
        0xE2,
        0x82
    };

    std::array<char16_t, 4> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf16>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Incomplete);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);

    CHECK(output[0] == u'A');
    CHECK(output[1] == u'B');
}

TEST_CASE("TextTranscoder reports invalid source input", "[core][text][transcode][edge][invalid]")
{
    constexpr std::array<std::uint8_t, 5> input{
        0x41,
        0x42,
        0xE2,
        0x28,
        0xA1
    };

    std::array<char32_t, 5> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf32>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Invalid);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);

    CHECK(output[0] == U'A');
    CHECK(output[1] == U'B');
}

TEST_CASE("TextTranscoder reports invalid UTF-16 source input", "[core][text][transcode][edge][invalid]")
{
    constexpr std::array<char16_t, 3> input{
        u'A',
        static_cast<char16_t>(0xD83D),
        u'B'
    };

    std::array<std::uint8_t, 8> output{};

    const auto result = TextTranscoder::transcode<Utf16, Utf8>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Invalid);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);
    CHECK(output[0] == 0x41);
}

TEST_CASE("TextTranscoder reports invalid UTF-32 source input", "[core][text][transcode][edge][invalid]")
{
    constexpr std::array<char32_t, 3> input{
        U'A',
        U'B',
        0xD800
    };

    std::array<std::uint8_t, 8> output{};

    const auto result = TextTranscoder::transcode<Utf32, Utf8>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Invalid);
    CHECK(result.inputConsumed == 2);
    CHECK(result.outputProduced == 2);

    CHECK(output[0] == 0x41);
    CHECK(output[1] == 0x42);
}

TEST_CASE("TextTranscoder reports output exhaustion without consuming the next scalar", "[core][text][transcode][edge][output-full]")
{
    constexpr std::array<char32_t, 2> input{
        U'A',
        0x1F600
    };

    std::array<std::uint8_t, 4> output{
        0xAA,
        0xBB,
        0xCC,
        0xDD
    };

    const auto result = TextTranscoder::transcode<Utf32, Utf8>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);

    CHECK(output[0] == 0x41);
    CHECK(output[1] == 0xBB);
    CHECK(output[2] == 0xCC);
    CHECK(output[3] == 0xDD);
}

TEST_CASE("TextTranscoder does not emit half a UTF-16 surrogate pair",
          "[core][text][transcode][edge][output-full]")
{
    constexpr std::array<char32_t, 2> input{
        U'A',
        0x1F600
    };

    std::array<char16_t, 2> output{
        static_cast<char16_t>(0xAAAA),
        static_cast<char16_t>(0xBBBB)
    };

    const auto result = TextTranscoder::transcode<Utf32, Utf16>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::OutputFull);
    CHECK(result.inputConsumed == 1);
    CHECK(result.outputProduced == 1);

    CHECK(output[0] == u'A');
    CHECK(output[1] == static_cast<char16_t>(0xBBBB));
}

TEST_CASE("TextTranscoder supports same-encoding UTF-8 transcoding",
          "[core][text][transcode][edge][same-encoding]")
{
    constexpr std::array<std::uint8_t, 8> input{
        0x41,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x9A, 0x80
    };

    std::array<std::uint8_t, 8> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf8>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());
    CHECK(output == input);
}

TEST_CASE("TextTranscoder supports same-encoding UTF-16 transcoding",
          "[core][text][transcode][edge][same-encoding]")
{
    constexpr std::array<char16_t, 4> input{
        u'A',
        static_cast<char16_t>(0x20AC),
        static_cast<char16_t>(0xD83D),
        static_cast<char16_t>(0xDE80)
    };

    std::array<char16_t, 4> output{};

    const auto result = TextTranscoder::transcode<Utf16, Utf16>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());
    CHECK(output == input);
}

TEST_CASE("TextTranscoder supports same-encoding UTF-32 transcoding",
          "[core][text][transcode][edge][same-encoding]")
{
    constexpr std::array<char32_t, 3> input{
        U'A',
        0x20AC,
        0x1F680
    };

    std::array<char32_t, 3> output{};

    const auto result = TextTranscoder::transcode<Utf32, Utf32>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());
    CHECK(output == input);
}

TEST_CASE("TextTranscoder operations are constexpr", "[core][text][transcode][constexpr]")
{
    constexpr auto result = [] {
        constexpr std::array<std::uint8_t, 4> input{
            0xF0, 0x9F, 0x9A, 0x80
        };

        std::array<char16_t, 2> output{};
        const auto transcodeResult = TextTranscoder::transcode<Utf8, Utf16>(input, output);

        return std::pair{output, transcodeResult};
    }();

    STATIC_CHECK(result.second.status == TextTranscodeStatus::Complete);
    STATIC_CHECK(result.second.inputConsumed == 4);
    STATIC_CHECK(result.second.outputProduced == 2);

    STATIC_CHECK(result.first[0] == static_cast<char16_t>(0xD83D));
    STATIC_CHECK(result.first[1] == static_cast<char16_t>(0xDE80));
}

TEST_CASE("TextTranscoder accepts source OutputFull after decoding one scalar",
          "[core][text][transcode][regression][output-full]")
{
    constexpr std::array<std::uint8_t, 2> input{
        0x41,
        0x42
    };

    std::array<char32_t, 2> output{};

    const auto result = TextTranscoder::transcode<Utf8, Utf32>(input, output);

    REQUIRE(result.status == TextTranscodeStatus::Complete);
    CHECK(result.inputConsumed == input.size());
    CHECK(result.outputProduced == output.size());

    CHECK(output[0] == U'A');
    CHECK(output[1] == U'B');
}
} // namespace job::core::text::tests