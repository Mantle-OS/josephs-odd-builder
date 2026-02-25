#include <thread>
#include <atomic>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <pty_io.h>

using namespace job::io;
using namespace std::chrono_literals;

TEST_CASE("PtyIO basic open/close lifecycle", "[pty]") {
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE(pty.isOpen());
    REQUIRE(pty.fd() != -1);
    REQUIRE_FALSE(pty.slaveName().empty());

    pty.closeDevice();
    REQUIRE_FALSE(pty.isOpen());
}

TEST_CASE("PtyIO local echo roundtrip", "[pty]") {
    PtyIO pty;
    REQUIRE(pty.openDevice());

    std::atomic<size_t> received{0};
    std::string buffer;

    pty.setLocalecho(true);
    pty.setReadCallback([&](const char *data, size_t len) {
        buffer.append(data, len);
        received += len;
    });

    const std::string msg = "ping\n";
    auto written = pty.write(msg.c_str(), msg.size());
    REQUIRE(written == static_cast<ssize_t>(msg.size()));

    std::this_thread::sleep_for(100ms);

    REQUIRE(received.load() > 0u);
    REQUIRE(buffer.find("ping") != std::string::npos);

    pty.closeDevice();
}

TEST_CASE("PtyIO starts a shell and receives output", "[pty][shell]") {
    PtyIO pty;
    REQUIRE(pty.openDevice());

    std::atomic<bool> gotOutput{false};

    pty.setReadCallback([&](const char *data, size_t len) {
        std::string chunk(data, len);
        if (chunk.find("hello") != std::string::npos)
            gotOutput = true;
    });

    pty.setExitCallback([](int status) {
        INFO("Shell exited with status " << status);
    });

    REQUIRE(pty.startShell("/bin/sh"));
    REQUIRE(pty.isOpen());

    const std::string cmd = "echo hello\n";

    SECTION("write() result is valid") {
        const ssize_t written = pty.write(cmd.c_str(), cmd.size());
        REQUIRE(written == static_cast<ssize_t>(cmd.size()));
    }

    // Wait for the echoed result
    for (int i = 0; i < 20 && !gotOutput.load(); ++i)
        std::this_thread::sleep_for(100ms);

    REQUIRE(gotOutput.load());

    pty.closeDevice();
    REQUIRE_FALSE(pty.isOpen());
}

TEST_CASE("PtyIO double close is safe", "[pty]") {
    PtyIO pty;
    REQUIRE(pty.openDevice());
    pty.closeDevice();
    REQUIRE_NOTHROW(pty.closeDevice());
}
