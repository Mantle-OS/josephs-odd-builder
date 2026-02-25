#include <catch2/catch_test_macros.hpp>
#include <mock_io.h>

using namespace job::io;

TEST_CASE("MockIO basic read/write lifecycle", "[mockio]") {
    MockIO io;
    REQUIRE(io.openDevice());
    REQUIRE(io.isOpen());

    io.setReadBuffer("abc123");
    char buf[10] = {0};
    REQUIRE(io.read(buf, 3) == 3);
    REQUIRE(std::string(buf, 3) == "abc");

    REQUIRE(io.write("xyz", 3) == 3);
    REQUIRE(io.peekWriteBuffer() == "xyz");

    io.closeDevice();
    REQUIRE_FALSE(io.isOpen());
}

TEST_CASE("MockIO takeWriteBuffer clears data", "[mockio]") {
    MockIO io;
    io.openDevice();
    io.write("hello", 5);
    REQUIRE(io.peekWriteBuffer() == "hello");
    REQUIRE(io.takeWriteBuffer() == "hello");
    REQUIRE(io.peekWriteBuffer().empty());
}

