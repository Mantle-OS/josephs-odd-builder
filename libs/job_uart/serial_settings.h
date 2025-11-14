#pragma once

#include <cstdint>
#include <string>
#include <termios.h>
#include <vector>

namespace job::uart{

struct SerialSettings
{
    std::string toString() const;
    std::string portName = "Unknown";
    uint32_t baudRate = 115200;
    speed_t toSpeed(uint32_t baudRate);

    uint8_t dataBits = 8;
    uint8_t dataBitsIndex = 3;
    uint32_t toDataBits(uint8_t bits);
    std::string dataBitString() const;


    enum class Parity : uint8_t {
        None,
        Even,
        Odd,
        Space,
        Mark
    };
    Parity parity = Parity::None;
    std::string parityString = "None";
    uint32_t toParity(Parity p, termios &tty);

    enum class StopBits : uint8_t {
        One,
        OneAndHalf,
        Two
    };
    StopBits stopBits = StopBits::One;
    std::string stopBitsString = "1";
    uint32_t toStopBits(StopBits sb);

    enum class FlowControl : uint8_t {
        None,
        RTSCTS,
        XONXOFF
    };
    FlowControl flowControl = FlowControl::None;
    std::string flowControlString = "None";
    uint32_t toFlowControl(FlowControl fc, termios &tty);

    bool localEcho = false;

    static const std::vector<uint32_t> &availableBaudRates();
    static const std::vector<uint8_t> &availableDataBits();
    static std::vector<std::string> availableParityOptions();
    static std::vector<std::string> availableStopBitsOptions();
    static std::vector<std::string> availableFlowControlOptions();

    void setBaudRateFromInt(int baud);
    void setDataBitsFromInt(int bits);
    void setParityFromString(const std::string &parity);
    void setStopBitsFromString(const std::string &stop);
    void setFlowControlFromString(const std::string &flow);

};

} // job::uart
// CHECKPOINT: v1
