#pragma once
#include <cstddef>
namespace job::ai::cords {
// A 16x16 Block. Targeted for AVX-512 (ZMM registers) or L1-Cache tiling.
// On AVX2 machines, this represents a "Super-Tile" composed of four Tile8s.
struct Tile16 {
    static constexpr std::size_t rows            = 16;
    static constexpr std::size_t cols            = 16;
    static constexpr std::size_t accumulation    = 1;
    static constexpr std::size_t elements        = rows * cols;      // 256 floats
    static constexpr std::size_t bytes           = elements * 4;     // 1024 bytes (1KB)
};
} // namespace job::ai::cords
