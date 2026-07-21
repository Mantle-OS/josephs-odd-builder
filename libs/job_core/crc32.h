#pragma once
#include <cstddef>
#include <cstdint>
#include <array>

namespace job::core {

consteval std::array<uint32_t, 256> generate_table() noexcept
{
    std::array<uint32_t, 256> table{};
    constexpr uint32_t polynomial = 0xEDB88320u; // polynomial

    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j)
            if (c & 1)
                c = polynomial ^ (c >> 1);
            else
                c = c >> 1;

        table[i] = c;
    }
    return table;
}

inline constexpr auto kCrc32Table = generate_table();

struct Crc32 {
    [[nodiscard]] inline static uint32_t compute(const uint8_t* data, std::size_t len) noexcept
    {
        uint32_t crc = 0xFFFFFFFFu; // 1s

        for (std::size_t i = 0; i < len; i++) {
            uint8_t index = (crc ^ data[i]) & 0xFF;
            crc = kCrc32Table[index] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFFu;
    }
};

}
