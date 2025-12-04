#pragma once
#include <cstdint>
#include <cstddef>

namespace job::ai::cords {

enum class LayoutType : uint8_t {
    RowMajor = 0,    // Last dimension is contiguous (C, NumPy, PyTorch default)
    ColMajor,        // First dimension is contiguous (Fortran, MATLAB)
    Strided,         // Explicit strides per dimension
    Tiled            // Z-Curve or Block-Linear (for texture/tensor cores)
};

struct LayoutUtils {
    [[nodiscard]] static constexpr size_t offset2D(size_t r, size_t c, size_t stride, LayoutType type)
    {
        if (type == LayoutType::RowMajor)
            return r * stride + c;
        else
            return c * stride + r;

    }
};
} // namespace job::ai::cords
