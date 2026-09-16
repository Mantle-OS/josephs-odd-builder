#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <job_text.h>

namespace job::core::text::tests {

static_assert(UtfStreamCodec<Utf8>);
static_assert(UtfStreamCodec<Utf16>);
static_assert(UtfStreamCodec<Utf32>);

static_assert(std::same_as<Utf8StreamBuf::CodecType, Utf8>);
static_assert(std::same_as<Utf16StreamBuf::CodecType, Utf16>);
static_assert(std::same_as<Utf32StreamBuf::CodecType, Utf32>);

static_assert(std::same_as<Utf8StreamBuf::Unit, std::uint8_t>);
static_assert(std::same_as<Utf16StreamBuf::Unit, char16_t>);
static_assert(std::same_as<Utf32StreamBuf::Unit, char32_t>);

static_assert(std::same_as<Utf8StreamBuf::CharType, char>);
static_assert(std::same_as<Utf16StreamBuf::CharType, char16_t>);
static_assert(std::same_as<Utf32StreamBuf::CharType, char32_t>);

TEST_CASE("UTF stream buffer starts empty", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
    CHECK(buffer.in_avail() == 0);
    CHECK(Utf8StreamBuf::TraitsType::eq_int_type(buffer.sgetc(),
                                                 Utf8StreamBuf::TraitsType::eof()));

    std::array<char, 8> output{};
    CHECK(buffer.sgetn(output.data(), static_cast<std::streamsize>(output.size())) == 0);
}

TEST_CASE("UTF-8 stream buffer feeds and reads bulk input", "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view input{"Hello JOB"};

    REQUIRE(buffer.feed(input.data(),
                        static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK_FALSE(buffer.empty());
    CHECK(buffer.size() == input.size());
    CHECK(buffer.in_avail() == static_cast<std::streamsize>(input.size()));

    std::array<char, input.size()> output{};

    REQUIRE(buffer.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(std::string_view{output.data(), output.size()} == input);
    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
    CHECK(buffer.in_avail() == 0);
}

TEST_CASE("UTF-8 stream buffer supports single character reads", "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view input{"JOB"};

    REQUIRE(buffer.feed(input.data(),
                        static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(Utf8StreamBuf::TraitsType::to_char_type(buffer.sgetc()) == 'J');
    CHECK(buffer.size() == 3);

    CHECK(Utf8StreamBuf::TraitsType::to_char_type(buffer.sbumpc()) == 'J');
    CHECK(buffer.size() == 2);

    CHECK(Utf8StreamBuf::TraitsType::to_char_type(buffer.sgetc()) == 'O');
    CHECK(Utf8StreamBuf::TraitsType::to_char_type(buffer.sbumpc()) == 'O');
    CHECK(Utf8StreamBuf::TraitsType::to_char_type(buffer.sbumpc()) == 'B');

    CHECK(buffer.empty());
    CHECK(Utf8StreamBuf::TraitsType::eq_int_type(buffer.sgetc(),
                                                 Utf8StreamBuf::TraitsType::eof()));
}

TEST_CASE("UTF-8 stream buffer preserves unread data across additional feeds",
          "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view first{"abcdef"};

    REQUIRE(buffer.feed(first.data(),
                        static_cast<std::streamsize>(first.size())) ==
            static_cast<std::streamsize>(first.size()));

    std::array<char, 3> prefix{};

    REQUIRE(buffer.sgetn(prefix.data(), static_cast<std::streamsize>(prefix.size())) == static_cast<std::streamsize>(prefix.size()));
    CHECK(std::string_view{prefix.data(), prefix.size()} == "abc");
    CHECK(buffer.size() == 3);

    const std::string second(4096, 'x');

    REQUIRE(buffer.feed(second.data(),
                        static_cast<std::streamsize>(second.size())) ==
            static_cast<std::streamsize>(second.size()));

    CHECK(buffer.size() == 3 + second.size());

    std::vector<char> output(buffer.size());

    REQUIRE(buffer.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(output.size()));

    REQUIRE(output.size() == 3 + second.size());

    CHECK(std::string_view{output.data(), 3} == "def");

    for (std::size_t i = 3; i < output.size(); ++i)
        CHECK(output[i] == 'x');

    CHECK(buffer.empty());
}

TEST_CASE("UTF stream buffer clear discards unread input", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view input{"data that should disappear"};

    REQUIRE(buffer.feed(input.data(),
                        static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    REQUIRE_FALSE(buffer.empty());

    buffer.clear();

    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
    CHECK(buffer.in_avail() == 0);
    CHECK(Utf8StreamBuf::TraitsType::eq_int_type(buffer.sgetc(),
                                                 Utf8StreamBuf::TraitsType::eof()));
}

TEST_CASE("UTF stream buffer handles empty feed input", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    CHECK(buffer.feed(nullptr, 0) == 0);
    CHECK(buffer.feed(nullptr, 4) == 0);

    const Utf8StreamBuf::ReadView empty{};
    CHECK(buffer.feed(empty) == 0);

    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
}

TEST_CASE("UTF-8 stream buffer writes through callback", "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;
    std::string captured;

    buffer.setWriteCallback([&captured](const char *data, std::streamsize count) {
        captured.append(data, static_cast<std::size_t>(count));
        return count;
    });

    constexpr std::string_view input{"Hello JOB"};

    REQUIRE(buffer.sputn(input.data(),
                         static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(captured == input);
}

TEST_CASE("UTF-8 stream buffer single character output uses streambuf interface",
          "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;
    std::string captured;

    buffer.setWriteCallback([&captured](const char *data, std::streamsize count) {
        captured.append(data, static_cast<std::size_t>(count));
        return count;
    });

    const auto result = buffer.sputc('J');

    REQUIRE_FALSE(Utf8StreamBuf::TraitsType::eq_int_type(result,
                                                         Utf8StreamBuf::TraitsType::eof()));

    CHECK(Utf8StreamBuf::TraitsType::to_char_type(result) == 'J');
    CHECK(captured == "J");
}

TEST_CASE("UTF stream buffer output reports missing callback", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view input{"JOB"};

    CHECK(buffer.sputn(input.data(),
                       static_cast<std::streamsize>(input.size())) == 0);

    CHECK(Utf8StreamBuf::TraitsType::eq_int_type(buffer.sputc('J'),
                                                 Utf8StreamBuf::TraitsType::eof()));
}

TEST_CASE("UTF stream buffer output preserves partial write count", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    buffer.setWriteCallback([](const char *, std::streamsize count) {
        return count / 2;
    });

    constexpr std::string_view input{"abcdefgh"};

    CHECK(buffer.sputn(input.data(),
                       static_cast<std::streamsize>(input.size())) == 4);
}

TEST_CASE("UTF stream buffer clamps excessive callback write count", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    buffer.setWriteCallback([](const char *, std::streamsize count) {
        return count + 100;
    });

    constexpr std::string_view input{"JOB"};

    CHECK(buffer.sputn(input.data(),
                       static_cast<std::streamsize>(input.size())) ==
          static_cast<std::streamsize>(input.size()));
}

TEST_CASE("UTF stream buffer treats failed callback write as zero", "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    buffer.setWriteCallback([](const char *, std::streamsize) {
        return static_cast<std::streamsize>(-1);
    });

    constexpr std::string_view input{"JOB"};

    CHECK(buffer.sputn(input.data(),
                       static_cast<std::streamsize>(input.size())) == 0);

    CHECK(Utf8StreamBuf::TraitsType::eq_int_type(buffer.sputc('J'),
                                                 Utf8StreamBuf::TraitsType::eof()));
}

TEST_CASE("UTF stream buffer sync succeeds without buffered output",
          "[core][text][streambuf]")
{
    Utf8StreamBuf buffer;

    CHECK(buffer.pubsync() == 0);
}

TEST_CASE("UTF-16 stream buffer preserves code units", "[core][text][streambuf][utf16]")
{
    Utf16StreamBuf buffer;

    constexpr std::array<char16_t, 6> input{
        u'J',
        u'O',
        u'B',
        0x03A9,
        0xD83D,
        0xDE00
    };

    REQUIRE(buffer.feed(input.data(),
                        static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    std::array<char16_t, input.size()> output{};

    REQUIRE(buffer.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(output == input);
    CHECK(buffer.empty());
}

TEST_CASE("UTF-32 stream buffer preserves code units", "[core][text][streambuf][utf32]")
{
    Utf32StreamBuf buffer;

    constexpr std::array<char32_t, 5> input{
        U'J',
        U'O',
        U'B',
        0x1F600,
        0x10FFFF
    };

    REQUIRE(buffer.feed(input.data(),
                        static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    std::array<char32_t, input.size()> output{};

    REQUIRE(buffer.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(output == input);
    CHECK(buffer.empty());
}

TEST_CASE("UTF-8 stream buffer works with std::istream", "[core][text][streambuf][utf8][stl]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view input{"hello from JOB\n"};

    REQUIRE(buffer.feed(input.data(), static_cast<std::streamsize>(input.size())) == static_cast<std::streamsize>(input.size()));

    std::istream stream(&buffer);

    std::string line;
    std::getline(stream, line);

    CHECK(line == "hello from JOB");
    CHECK(buffer.empty());
}

TEST_CASE("UTF-8 stream buffer works with std::ostream", "[core][text][streambuf][utf8][stl]")
{
    Utf8StreamBuf buffer;
    std::string captured;

    buffer.setWriteCallback([&captured](const char *data, std::streamsize count) {
        captured.append(data, static_cast<std::size_t>(count));
        return count;
    });

    std::ostream stream(&buffer);
    stream << "JOB"
           << ' '
           << 42;

    stream.flush();

    CHECK(stream.good());
    CHECK(captured == "JOB 42");
}

TEST_CASE("UTF-8 stream buffer accepts new input after complete consumption",
          "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;

    constexpr std::string_view first{"first"};
    constexpr std::string_view second{"second"};

    REQUIRE(buffer.feed(first.data(), static_cast<std::streamsize>(first.size())) ==
            static_cast<std::streamsize>(first.size()));

    std::array<char, first.size()> firstOutput{};

    REQUIRE(buffer.sgetn(firstOutput.data(),
                         static_cast<std::streamsize>(firstOutput.size())) ==
            static_cast<std::streamsize>(first.size()));

    CHECK(std::string_view{firstOutput.data(), firstOutput.size()} == first);
    REQUIRE(buffer.empty());

    REQUIRE(buffer.feed(second.data(), static_cast<std::streamsize>(second.size())) ==
            static_cast<std::streamsize>(second.size()));

    std::array<char, second.size()> secondOutput{};

    REQUIRE(buffer.sgetn(secondOutput.data(),
                         static_cast<std::streamsize>(secondOutput.size())) ==
            static_cast<std::streamsize>(second.size()));

    CHECK(std::string_view{secondOutput.data(), secondOutput.size()} == second);
    CHECK(buffer.empty());
}

TEST_CASE("UTF-8 stream buffer preserves embedded zero and arbitrary byte values",
          "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;

    constexpr std::array<char, 6> input{
        'J',
        '\0',
        static_cast<char>(0xF0),
        static_cast<char>(0x9F),
        static_cast<char>(0x80),
        static_cast<char>(0xFF)
    };

    REQUIRE(buffer.feed(input.data(),
                        static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    std::array<char, input.size()> output{};

    REQUIRE(buffer.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(output.size()));

    CHECK(output == input);
}
TEST_CASE("UTF-8 stream buffer preserves units across split feed boundaries",
          "[core][text][streambuf][utf8]")
{
    Utf8StreamBuf buffer;

    constexpr std::array<char, 2> first{
        static_cast<char>(0xF0),
        static_cast<char>(0x9F)
    };

    constexpr std::array<char, 2> second{
        static_cast<char>(0x98),
        static_cast<char>(0x80)
    };

    REQUIRE(buffer.feed(first.data(),
                        static_cast<std::streamsize>(first.size())) ==
            static_cast<std::streamsize>(first.size()));

    REQUIRE(buffer.feed(second.data(),
                        static_cast<std::streamsize>(second.size())) ==
            static_cast<std::streamsize>(second.size()));

    constexpr std::array<char, 4> expected{
        static_cast<char>(0xF0),
        static_cast<char>(0x9F),
        static_cast<char>(0x98),
        static_cast<char>(0x80)
    };

    std::array<char, expected.size()> output{};

    REQUIRE(buffer.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(output.size()));

    CHECK(output == expected);
}

TEST_CASE("UTF-16 stream buffer preserves surrogate pair across split feed boundaries",
          "[core][text][streambuf][utf16]")
{
    Utf16StreamBuf buffer;

    constexpr char16_t high = 0xD83D;
    constexpr char16_t low = 0xDE00;

    REQUIRE(buffer.feed(&high, 1) == 1);
    REQUIRE(buffer.feed(&low, 1) == 1);

    std::array<char16_t, 2> output{};

    REQUIRE(buffer.sgetn(output.data(), 2) == 2);

    CHECK(output[0] == high);
    CHECK(output[1] == low);
}

TEST_CASE("UTF stream buffer factories construct concrete stream buffers",
          "[core][text][streambuf]")
{
    const auto shared = Utf8StreamBuf::createShared();
    const auto unique = Utf8StreamBuf::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    CHECK(shared->empty());
    CHECK(unique->empty());
}

} // namespace job::core::text::tests