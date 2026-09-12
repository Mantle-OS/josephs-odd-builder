#pragma once

#include <cstdint>
#include <utility>

namespace job::usb::tests {
static constexpr std::uint16_t test_vendors[] = {
    0x046d, // Logitech
    0x1532, // Razer
    0x045e, // Microsoft
    0x05ac, // Apple
    0x1d6b, // Linux Foundation
    0x8087, // Intel
    0x0bda, // Realtek
    0x18d1  // Google
};
static constexpr std::pair<std::uint16_t, std::uint16_t> test_products[] = {
    {0x046d, 0xc534},
    {0x1d6b, 0x0002},
    {0x045e, 0x028e},
    {0x1532, 0x0064}
};
}