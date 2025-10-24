#include "serial_settings.h"
#include <map>
#include <algorithm>
namespace job::io{

static const std::vector<uint32_t> kBaudRates = {
    300, 600, 1200, 2400, 4800, 9600,
    19200, 38400, 57600, 115200, 230400,
    460800, 576000, 921600, 1000000,
    1500000, 2000000
};

static const std::vector<uint8_t> kDataBits = { 5, 6, 7, 8 };

static const std::map<std::string, SerialSettings::Parity> kParityMap = {
    {"None",  SerialSettings::Parity::None},
    {"Even",  SerialSettings::Parity::Even},
    {"Odd",   SerialSettings::Parity::Odd},
    {"Space", SerialSettings::Parity::Space},
    {"Mark",  SerialSettings::Parity::Mark}
};

static const std::map<std::string, SerialSettings::StopBits> kStopBitsMap = {
    {"1",   SerialSettings::StopBits::One},
    {"1.5", SerialSettings::StopBits::OneAndHalf},
    {"2",   SerialSettings::StopBits::Two}
};

static const std::map<std::string, SerialSettings::FlowControl> kFlowControlMap = {
    {"None",     SerialSettings::FlowControl::None},
    {"RTS/CTS",  SerialSettings::FlowControl::RTSCTS},
    {"XON/XOFF", SerialSettings::FlowControl::XONXOFF}
};

speed_t SerialSettings::toSpeed(uint32_t baudRate)
{
    switch (baudRate) {
    case 50:
        return B50;
    case 75:
        return B75;
    case 110:
        return B110;
    case 134:
        return B134;
    case 150:
        return B150;
    case 200:
        return B200;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
#ifdef B460800
    case 460800: return B460800;
#endif
#ifdef B921600
    case 921600: return B921600;
#endif
    default:
        return B115200;
    }
}

uint32_t SerialSettings::toDataBits(uint8_t bits)
{
    switch (bits) {
    case 5:
        return CS5;
    case 6:
        return CS6;
    case 7:
        return CS7;
    case 8:
        return CS8;
    default:
        return CS8;
    }
}

std::string SerialSettings::dataBitString() const
{
    return std::to_string(dataBits);
}

uint32_t SerialSettings::toParity(Parity p, termios &tty)
{
    switch (p) {
    case Parity::None:
        tty.c_iflag &= ~INPCK;
        return 0;
    case Parity::Even:
        tty.c_iflag |= INPCK;
        return PARENB;
    case Parity::Odd:
        tty.c_iflag |= INPCK;
        return PARENB | PARODD;
    case Parity::Space:
    case Parity::Mark:
        // Not supported in termios. Needs special handling or ioctl extensions.
        return 0;
    default:
        return 0;
    }
}

uint32_t SerialSettings::toStopBits(StopBits sb)
{
    switch (sb) {
    case StopBits::One:
        return 0;
    case StopBits::Two:
        return CSTOPB;
    case StopBits::OneAndHalf:
        return CSTOPB; // not directly supported
    default:
        return 0;
    }
}

uint32_t SerialSettings::toFlowControl(FlowControl fc, termios &tty)
{
    switch (fc) {
    case FlowControl::None:
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_cflag &= ~CRTSCTS;
        return 0;
    case FlowControl::RTSCTS:
        tty.c_cflag |= CRTSCTS;
        return 0;
    case FlowControl::XONXOFF:
        tty.c_iflag |= (IXON | IXOFF | IXANY);
        return 0;
    default:
        return 0;
    }
}

const std::vector<uint32_t> &SerialSettings::availableBaudRates()
{
    return kBaudRates;
}

const std::vector<uint8_t> &SerialSettings::availableDataBits()
{
    return kDataBits;
}

std::vector<std::string> SerialSettings::availableParityOptions()
{
    std::vector<std::string> ret;
    for (const auto &[key, _] : kParityMap)
        ret.push_back(key);
    return ret;
}

std::vector<std::string> SerialSettings::availableStopBitsOptions()
{
    std::vector<std::string> ret;
    for (const auto &[key, _] : kStopBitsMap)
        ret.push_back(key);
    return ret;
}

std::vector<std::string> SerialSettings::availableFlowControlOptions()
{
    std::vector<std::string> ret;
    for (const auto &[key, _] : kFlowControlMap)
        ret.push_back(key);
    return ret;
}


void SerialSettings::setBaudRateFromInt(int baud)
{
    if (std::find(kBaudRates.begin(), kBaudRates.end(), baud) != kBaudRates.end())
        baudRate = static_cast<uint32_t>(baud);
}

void SerialSettings::setDataBitsFromInt(int bits)
{
    auto it = std::find(kDataBits.begin(), kDataBits.end(), bits);
    if (it != kDataBits.end()) {
        dataBits = static_cast<uint8_t>(bits);
        dataBitsIndex = static_cast<uint8_t>(std::distance(kDataBits.begin(), it));
    }
}

void SerialSettings::setParityFromString(const std::string &parityStr)
{
    if (auto it = kParityMap.find(parityStr); it != kParityMap.end()) {
        parity = it->second;
        parityString = it->first;
    }
}

void SerialSettings::setStopBitsFromString(const std::string &stopStr)
{
    if (auto it = kStopBitsMap.find(stopStr); it != kStopBitsMap.end()) {
        stopBits = it->second;
        stopBitsString = it->first;
    }
}

void SerialSettings::setFlowControlFromString(const std::string &flowStr)
{
    if (auto it = kFlowControlMap.find(flowStr); it != kFlowControlMap.end()) {
        flowControl = it->second;
        flowControlString = it->first;
    }
}

std::string SerialSettings::toString() const
{
    auto dbStr = dataBitString();
    return portName + " " +
           std::to_string(baudRate) + "bps " +
           dbStr + "N" +
           stopBitsString + " " +
           flowControlString;
}

}
