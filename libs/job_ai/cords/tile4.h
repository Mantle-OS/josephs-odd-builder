#pragma once
#include <cstddef>
namespace job::ai::cords {
// A 4x4 Register Block.
// Fits into 4x SSE registers (128-bit).
struct Tile4 {
    static constexpr std::size_t rows            = 4;
    static constexpr std::size_t cols            = 4;
    static constexpr std::size_t accumulation    = 1;
    static constexpr std::size_t elements        = rows * cols;     // 16 floats
    static constexpr std::size_t bytes           = elements * 4;
};
} // namespace job::ai::cords
