#include <catch2/catch_test_macros.hpp>

#include <io_factory.h>
#include <file_io.h>
#include <pty_io.h>

using namespace job::io;

TEST_CASE("IOFactory creates FileIO correctly", "[iofactory][file]") {
    auto io = IOFactory::createFromURI("file:/tmp/testfile.txt");
    REQUIRE(io != nullptr);
    REQUIRE(dynamic_cast<FileIO*>(io.get()) != nullptr);
}

TEST_CASE("IOFactory creates PTY backend", "[iofactory][pty]") {
    auto io = IOFactory::createFromURI("pty:");
    REQUIRE(io != nullptr);
    REQUIRE(dynamic_cast<PtyIO*>(io.get()) != nullptr);
}

// TEST_CASE("IOFactory creates Serial backend", "[iofactory][serial]") {
//     auto io = IOFactory::createFromURI("serial:/dev/ttyUSB0");
//     REQUIRE(io != nullptr);
//     REQUIRE(dynamic_cast<SerialIO*>(io.get()) != nullptr);
// }

TEST_CASE("IOFactory rejects bad URIs", "[iofactory][invalid]") {
    REQUIRE_THROWS_AS(IOFactory::createFromURI("invaliduri"), std::invalid_argument);
    REQUIRE_THROWS_AS(IOFactory::createFromURI("foo:bar"), std::runtime_error);
}
