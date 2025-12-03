#pragma once
#include <cstddef>
#include <cstdint>
namespace job::core {
struct Crc32 {
    [[nodiscard]] static uint32_t compute(const uint8_t *data, std::size_t len) noexcept;
};
}
