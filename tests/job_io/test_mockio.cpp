#include <array>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <mock_io.h>

using namespace job::io;

TEST_CASE("MockIO basic read/write lifecycle", "[mockio]")
{
    MockIO io;

    CHECK_FALSE(io.isOpen());
    CHECK_FALSE(io.readable());
    CHECK_FALSE(io.writable());

    REQUIRE(io.openDevice());

    CHECK(io.isOpen());
    CHECK_FALSE(io.readable());
    CHECK(io.writable());

    io.setReadBuffer("abc123");

    CHECK(io.readable());

    std::array<char, 3> buffer{};

    REQUIRE(io.read(buffer.data(), buffer.size()) == 3);
    CHECK(std::string{buffer.data(), buffer.size()} == "abc");
    CHECK(io.readable());

    REQUIRE(io.write("xyz", 3) == 3);
    CHECK(io.peekWriteBuffer() == "xyz");

    io.closeDevice();

    CHECK_FALSE(io.isOpen());
    CHECK_FALSE(io.readable());
    CHECK_FALSE(io.writable());
    CHECK(io.peekWriteBuffer().empty());
}

TEST_CASE("MockIO read buffer supports set append and clear", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    io.setReadBuffer("abc");
    io.appendReadBuffer("def");

    std::array<char, 6> buffer{};

    REQUIRE(io.read(buffer.data(), buffer.size()) == 6);
    CHECK(std::string{buffer.data(), buffer.size()} == "abcdef");
    CHECK_FALSE(io.readable());

    io.setReadBuffer("123");

    CHECK(io.readable());

    io.clearReadBuffer();

    CHECK_FALSE(io.readable());
    CHECK(io.read(buffer.data(), buffer.size()) == 0);
}

TEST_CASE("MockIO takeWriteBuffer clears data", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    REQUIRE(io.write("hello", 5) == 5);

    CHECK(io.peekWriteBuffer() == "hello");
    CHECK(io.takeWriteBuffer() == "hello");
    CHECK(io.peekWriteBuffer().empty());
}

TEST_CASE("MockIO closed device rejects read and write", "[mockio]")
{
    MockIO io;

    std::array<char, 4> buffer{};

    CHECK(io.read(buffer.data(), buffer.size()) == -1);
    CHECK(io.write("JOB", 3) == -1);
}

TEST_CASE("MockIO zero length read and write are successful no-ops", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    char value{};

    CHECK(io.read(&value, 0) == 0);
    CHECK(io.write("", 0) == 0);
    CHECK(io.peekWriteBuffer().empty());
}

TEST_CASE("MockIO rejects null read and write buffers", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    CHECK(io.read(nullptr, 1) == -1);
    CHECK(io.write(nullptr, 1) == -1);
}

TEST_CASE("MockIO nonblocking state can be configured", "[mockio]")
{
    MockIO io;

    CHECK_FALSE(io.nonBlocking());

    io.setNonBlocking(true);
    CHECK(io.nonBlocking());

    io.setNonBlocking(false);
    CHECK_FALSE(io.nonBlocking());
}

TEST_CASE("MockIO read callback receives emitted data", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    std::string received;

    io.setReadCallback([&received](const char *data, std::size_t size) {
        received.assign(data, size);
    });

    io.emitRead("hello");

    CHECK(received == "hello");
}

TEST_CASE("MockIO does not emit read callback while closed", "[mockio]")
{
    MockIO io;

    bool called = false;

    io.setReadCallback([&called](const char *, std::size_t) {
        called = true;
    });

    io.emitRead("hello");

    CHECK_FALSE(called);
}

TEST_CASE("MockIO read callback can reenter MockIO", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    io.setReadCallback([&io](const char *data, std::size_t size) {
        REQUIRE(io.isOpen());
        REQUIRE(io.write(data, size) == static_cast<ssize_t>(size));
    });

    io.emitRead("JOB");

    CHECK(io.peekWriteBuffer() == "JOB");
}

TEST_CASE("MockIO close clears callback and buffers", "[mockio]")
{
    MockIO io;
    REQUIRE(io.openDevice());

    bool called = false;

    io.setReadBuffer("input");
    REQUIRE(io.write("output", 6) == 6);

    io.setReadCallback([&called](const char *, std::size_t) {
        called = true;
    });

    io.closeDevice();

    CHECK_FALSE(io.isOpen());
    CHECK_FALSE(io.readable());
    CHECK(io.peekWriteBuffer().empty());

    REQUIRE(io.openDevice());

    io.emitRead("hello");

    CHECK_FALSE(called);
}

TEST_CASE("MockIO factories construct usable instances", "[mockio]")
{
    const auto shared = MockIO::createShared();
    const auto unique = MockIO::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    CHECK_FALSE(shared->isOpen());
    CHECK_FALSE(unique->isOpen());

    REQUIRE(shared->openDevice());
    REQUIRE(unique->openDevice());

    CHECK(shared->isOpen());
    CHECK(unique->isOpen());
}