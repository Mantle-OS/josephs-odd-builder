#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace job::ai::cords {

enum class LayoutType : std::uint8_t {
    RowMajor = 0,    // Last dimension is contiguous (C, NumPy, PyTorch default)
    ColMajor,        // First dimension is contiguous (Fortran, MATLAB)
    Strided,         // Explicit strides per dimension
    Tiled            // Z-Curve or Block-Linear (for texture/tensor cores)
};

struct LayoutUtils {
    [[nodiscard]] static constexpr std::size_t offset2D(std::size_t row,
                                                        std::size_t col,
                                                        std::size_t stride,
                                                        LayoutType type)
    {
        switch (type) {
        case LayoutType::RowMajor:
            // row-major: stride is typically "numCols"
            return row * stride + col;

        case LayoutType::ColMajor:
            // column-major: stride is typically "numRows"
            return col * stride + row;

        case LayoutType::Strided:
        case LayoutType::Tiled:
            // These layouts need per-dimension strides / tile info.
            // Calling offset2D with them is a contract violation.
            assert(false && "LayoutUtils::offset2D: Strided/Tiled layouts "
                            "require explicit per-dimension strides.");
            return 0;
        }

        // Highway to the "should never get here" zone.
        assert(false && "LayoutUtils::offset2D: unknown LayoutType");
        return 0;
    }
};

} // namespace job::ai::cords
