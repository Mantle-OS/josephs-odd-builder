#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace job::io{

class SerialIO;

namespace serial_info {

inline constexpr const char *kCandidatePrefixes[] = {
    "ttyS",    // Standard UART
    "ttyUSB",  // USB converters
    "ttyACM",  // CDC ACM (modems)
    "ttyAMA",  // AMBA serial (e.g., Raspberry Pi)
    "ttyGS",   // Gadget serial
    "ttyMI",   // MOXA
    "ttymxc",  // Freescale/NXP i.MX
    "ttyTHS",  // Tegra
    "ttyO",    // OMAP
    "rfcomm",  // Bluetooth
    "ircomm",  // Infrared
    "tnt"      // Virtual null modem (tty0tty)
};


[[nodiscard]] std::vector<std::string> scanSerialPortsFromFilesystem();
void update_serial_devices(std::map<std::string, std::unique_ptr<SerialIO>> &deviceMap);

}
}
