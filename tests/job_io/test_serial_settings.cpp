#include <catch2/catch_test_macros.hpp>
#include "serial/serial_settings.h"
#include <termios.h>

using namespace job::io;

TEST_CASE("SerialSettings default values are sane", "[serial][defaults]") {
    SerialSettings s;
    REQUIRE(s.baudRate == 115200);
    REQUIRE(s.dataBits == 8);
    REQUIRE(s.parity == SerialSettings::Parity::None);
    REQUIRE(s.stopBits == SerialSettings::StopBits::One);
    REQUIRE(s.flowControl == SerialSettings::FlowControl::None);
}

TEST_CASE("Baud rate conversions work", "[serial][baud]") {
    SerialSettings s;
    REQUIRE(s.toSpeed(9600) == B9600);
    REQUIRE(s.toSpeed(19200) == B19200);
    REQUIRE(s.toSpeed(115200) == B115200);
    REQUIRE(s.toSpeed(230400) == B230400);

    // Unknown baud should fallback safely
    REQUIRE(s.toSpeed(12345) == B115200);
}

TEST_CASE("Data bits conversions work", "[serial][databits]") {
    SerialSettings s;
    REQUIRE(s.toDataBits(5) == CS5);
    REQUIRE(s.toDataBits(6) == CS6);
    REQUIRE(s.toDataBits(7) == CS7);
    REQUIRE(s.toDataBits(8) == CS8);
}

TEST_CASE("Parity configuration modifies termios flags correctly", "[serial][parity]") {
    SerialSettings s;
    struct termios tty{};
    tty.c_iflag = 0;
    tty.c_cflag = 0;

    SECTION("No parity disables INPCK") {
        auto flags = s.toParity(SerialSettings::Parity::None, tty);
        REQUIRE((tty.c_iflag & INPCK) == 0);
        REQUIRE(flags == 0);
    }

    SECTION("Even parity enables INPCK and PARENB") {
        auto flags = s.toParity(SerialSettings::Parity::Even, tty);
        REQUIRE((tty.c_iflag & INPCK) != 0);
        REQUIRE(flags == PARENB);
    }

    SECTION("Odd parity enables INPCK and PARODD") {
        auto flags = s.toParity(SerialSettings::Parity::Odd, tty);
        REQUIRE((tty.c_iflag & INPCK) != 0);
        REQUIRE((flags & PARENB) != 0);
        REQUIRE((flags & PARODD) != 0);
    }
}

TEST_CASE("Stop bits conversion works", "[serial][stopbits]") {
    SerialSettings s;
    REQUIRE(s.toStopBits(SerialSettings::StopBits::One) == 0);
    REQUIRE(s.toStopBits(SerialSettings::StopBits::Two) == CSTOPB);
    REQUIRE(s.toStopBits(SerialSettings::StopBits::OneAndHalf) == CSTOPB);
}

TEST_CASE("Flow control flags behave correctly", "[serial][flow]") {
    SerialSettings s;
    struct termios tty{};
    tty.c_iflag = 0;
    tty.c_cflag = 0;

    SECTION("None disables IXON/IXOFF/IXANY/CRTSCTS") {
        s.toFlowControl(SerialSettings::FlowControl::None, tty);
        REQUIRE((tty.c_iflag & (IXON | IXOFF | IXANY)) == 0);
        REQUIRE((tty.c_cflag & CRTSCTS) == 0);
    }

    SECTION("RTS/CTS enables CRTSCTS") {
        s.toFlowControl(SerialSettings::FlowControl::RTSCTS, tty);
        REQUIRE((tty.c_cflag & CRTSCTS) != 0);
    }

    SECTION("XON/XOFF enables software flow control flags") {
        s.toFlowControl(SerialSettings::FlowControl::XONXOFF, tty);
        REQUIRE((tty.c_iflag & (IXON | IXOFF | IXANY)) != 0);
    }
}

TEST_CASE("Setters from string and int apply correctly", "[serial][mutators]") {
    SerialSettings s;
    s.setBaudRateFromInt(9600);
    REQUIRE(s.baudRate == 9600);

    s.setDataBitsFromInt(7);
    REQUIRE(s.dataBits == 7);

    s.setParityFromString("Even");
    REQUIRE(s.parity == SerialSettings::Parity::Even);
    REQUIRE(s.parityString == "Even");

    s.setStopBitsFromString("2");
    REQUIRE(s.stopBits == SerialSettings::StopBits::Two);
    REQUIRE(s.stopBitsString == "2");

    s.setFlowControlFromString("RTS/CTS");
    REQUIRE(s.flowControl == SerialSettings::FlowControl::RTSCTS);
    REQUIRE(s.flowControlString == "RTS/CTS");
}

TEST_CASE("Available options lists are non-empty", "[serial][lists]") {
    REQUIRE_FALSE(SerialSettings::availableBaudRates().empty());
    REQUIRE_FALSE(SerialSettings::availableDataBits().empty());
    REQUIRE_FALSE(SerialSettings::availableParityOptions().empty());
    REQUIRE_FALSE(SerialSettings::availableStopBitsOptions().empty());
    REQUIRE_FALSE(SerialSettings::availableFlowControlOptions().empty());
}
