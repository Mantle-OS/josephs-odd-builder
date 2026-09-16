#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_text.h>
#include <mock_io.h>

namespace job::core::text::tests {

TEST_CASE("MockIO read callback feeds UTF-8 stream buffer",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    io.setReadCallback([&stream](const char *data, std::size_t size) {
        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    io.emitRead("Hello JOB");

    CHECK(stream.size() == 9);

    std::array<char, 9> output{};

    REQUIRE(stream.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) ==
            static_cast<std::streamsize>(output.size()));

    CHECK(std::string_view{output.data(), output.size()} == "Hello JOB");
    CHECK(stream.empty());
}

TEST_CASE("UTF-8 stream buffer writes through MockIO",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    stream.setWriteCallback([&io](const char *data, std::streamsize count) {
        const ssize_t written = io.write(data, static_cast<std::size_t>(count));

        if (written < 0)
            return static_cast<std::streamsize>(0);

        return static_cast<std::streamsize>(written);
    });

    constexpr std::string_view input{"Hello JOB"};

    REQUIRE(stream.sputn(input.data(),
                         static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(io.peekWriteBuffer() == input);
}

TEST_CASE("MockIO and UTF-8 stream buffer form a bidirectional bridge",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    io.setReadCallback([&stream](const char *data, std::size_t size) {
        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    stream.setWriteCallback([&io](const char *data, std::streamsize count) {
        const ssize_t written = io.write(data, static_cast<std::size_t>(count));

        if (written < 0)
            return static_cast<std::streamsize>(0);

        return static_cast<std::streamsize>(written);
    });

    io.emitRead("input");

    std::array<char, 5> input{};

    REQUIRE(stream.sgetn(input.data(),
                         static_cast<std::streamsize>(input.size())) ==
            static_cast<std::streamsize>(input.size()));

    CHECK(std::string_view{input.data(), input.size()} == "input");

    REQUIRE(stream.sputn("output", 6) == 6);

    CHECK(io.peekWriteBuffer() == "output");
}

TEST_CASE("MockIO callback boundaries do not become UTF-8 boundaries",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    io.setReadCallback([&stream](const char *data, std::size_t size) {
        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    const std::string first{
        static_cast<char>(0xF0),
        static_cast<char>(0x9F)
    };

    const std::string second{
        static_cast<char>(0x98),
        static_cast<char>(0x80)
    };

    io.emitRead(first);
    io.emitRead(second);

    CHECK(stream.size() == 4);

    std::array<char, 4> output{};

    REQUIRE(stream.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) == 4);

    CHECK(static_cast<unsigned char>(output[0]) == 0xF0);
    CHECK(static_cast<unsigned char>(output[1]) == 0x9F);
    CHECK(static_cast<unsigned char>(output[2]) == 0x98);
    CHECK(static_cast<unsigned char>(output[3]) == 0x80);
}

TEST_CASE("MockIO can emit multiple reads into UTF-8 stream buffer",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    io.setReadCallback([&stream](const char *data, std::size_t size) {
        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    io.emitRead("abc");
    io.emitRead("def");
    io.emitRead("ghi");

    CHECK(stream.size() == 9);

    std::array<char, 9> output{};

    REQUIRE(stream.sgetn(output.data(),
                         static_cast<std::streamsize>(output.size())) == 9);

    CHECK(std::string_view{output.data(), output.size()} == "abcdefghi");
}

TEST_CASE("MockIO read bridge preserves unread stream data across later callbacks",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    io.setReadCallback([&stream](const char *data, std::size_t size) {
        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    io.emitRead("abcdef");

    std::array<char, 3> prefix{};

    REQUIRE(stream.sgetn(prefix.data(),
                         static_cast<std::streamsize>(prefix.size())) == 3);

    CHECK(std::string_view{prefix.data(), prefix.size()} == "abc");

    io.emitRead("ghijkl");

    std::array<char, 9> remainder{};

    REQUIRE(stream.sgetn(remainder.data(),
                         static_cast<std::streamsize>(remainder.size())) == 9);

    CHECK(std::string_view{remainder.data(), remainder.size()} == "defghijkl");
    CHECK(stream.empty());
}

TEST_CASE("MockIO closed state prevents data entering UTF-8 stream buffer",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    bool callbackCalled = false;

    io.setReadCallback([&stream, &callbackCalled](const char *data, std::size_t size) {
        callbackCalled = true;

        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    io.emitRead("ignored");

    CHECK_FALSE(callbackCalled);
    CHECK(stream.empty());
    CHECK(stream.size() == 0);
}

TEST_CASE("UTF-8 stream buffer reports failed MockIO output after close",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    stream.setWriteCallback([&io](const char *data, std::streamsize count) {
        const ssize_t written = io.write(data, static_cast<std::size_t>(count));

        if (written < 0)
            return static_cast<std::streamsize>(0);

        return static_cast<std::streamsize>(written);
    });

    io.closeDevice();

    CHECK(stream.sputn("JOB", 3) == 0);
    CHECK(io.peekWriteBuffer().empty());
}

TEST_CASE("MockIO nonblocking state does not change UTF-8 bridge semantics",
          "[core][text][streambuf][io][utf8]")
{
    job::io::MockIO io;
    Utf8StreamBuf stream;

    REQUIRE(io.openDevice());

    io.setNonBlocking(true);
    REQUIRE(io.nonBlocking());

    io.setReadCallback([&stream](const char *data, std::size_t size) {
        const auto fed = stream.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    stream.setWriteCallback([&io](const char *data, std::streamsize count) {
        const ssize_t written = io.write(data, static_cast<std::size_t>(count));

        if (written < 0)
            return static_cast<std::streamsize>(0);

        return static_cast<std::streamsize>(written);
    });

    io.emitRead("read");

    std::array<char, 4> input{};

    REQUIRE(stream.sgetn(input.data(),
                         static_cast<std::streamsize>(input.size())) == 4);

    CHECK(std::string_view{input.data(), input.size()} == "read");

    REQUIRE(stream.sputn("write", 5) == 5);
    CHECK(io.peekWriteBuffer() == "write");
}

TEST_CASE("MockIO bridge works through std::istream and std::ostream",
          "[core][text][streambuf][io][utf8][stl]")
{
    job::io::MockIO io;
    Utf8StreamBuf buffer;

    REQUIRE(io.openDevice());

    io.setReadCallback([&buffer](const char *data, std::size_t size) {
        const auto fed = buffer.feed(data, static_cast<std::streamsize>(size));
        REQUIRE(fed == static_cast<std::streamsize>(size));
    });

    buffer.setWriteCallback([&io](const char *data, std::streamsize count) {
        const ssize_t written = io.write(data, static_cast<std::size_t>(count));

        if (written < 0)
            return static_cast<std::streamsize>(0);

        return static_cast<std::streamsize>(written);
    });

    io.emitRead("hello from mock io\n");

    std::istream input(&buffer);

    std::string line;
    std::getline(input, line);

    CHECK(line == "hello from mock io");

    std::ostream output(&buffer);

    output << "hello from job text";
    output.flush();

    CHECK(output.good());
    CHECK(io.peekWriteBuffer() == "hello from job text");
}

} // namespace job::core::text::tests