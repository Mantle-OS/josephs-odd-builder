#pragma once
#include <cstddef>
namespace job::ai::cords {
// The Golden Goose !!!
// An 8x8 Register Block.
// The "Golden Ratio" for AVX2 (256-bit).
// Uses 16 YMM registers (8 accumulators, 8 temps/loads).
struct Tile8 {
    static constexpr size_t rows            = 8;
    static constexpr size_t cols            = 8;
    static constexpr size_t accumulation    = 1;
    static constexpr size_t elements        = rows * cols;      // 64 floats
    static constexpr size_t bytes           = elements * 4;     // 256 bytes
};

} // namespace job::ai::cords
