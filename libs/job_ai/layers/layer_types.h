#pragma once

#include "view.h"
#include <cstdint>

namespace job::ai::layers {

enum class LayerType : uint8_t {
    Input       = 0,    // Input.
    Dense       = 1,    // Standard Linear
    SparseMoE   = 2,    // Mixture of Experts
    Attention   = 3,    // Self/Cross Attention
    Embedding   = 4,    // Token -> Vector
    LayerNorm   = 5,    // Normalization
    RMSNorm     = 6,    // LLaMA style norm
    Residual    = 7,    // Add
    LinearLoRA  = 8,    // Add
    Output      = 9,    // Logits
    Abstract     = 254  // Unknown slash bad layer type
};

// Initializer types for weights
enum class InitType : uint8_t {
    Zero        = 0,    // All weights = 0
    One         = 1,    // All weights = 1
    Uniform     = 2,    // U(-a, a), params TBD
    Normal      = 3,    // N(0, σ²), params TBD
    Xavier      = 4,    // Glorot: fan-in/out based
    Kaiming     = 5,    // He: ReLU-friendly
    Lecun       = 6     // LeCun: SELU-friendly
};

enum class LayerMode : uint8_t {
    Training    = 0,   // The Layer is currently in training mode
    Inference   = 1    // The Layer is not Training and is in Question mode
};


enum LayerFlags : uint8_t {
    Causal    = 1u << 0,
    HasRouter = 1u << 1,
    HasBias   = 1u << 2,
};

// HACK for Dense layer to check if the dimension is 2D or 3D
inline void inferDenseShape(const cords::ViewR &input,
                            std::size_t &rows,
                            std::size_t &inFeatures)
{
    const auto &ext = input.extent();
    const auto rank = ext.rank();
    assert(rank >= 1 && "inferDenseShape: rank must be >= 1");

    // Last dimension is the feature dimension
    inFeatures = ext[rank - 1];

    // All preceding dims are “batch-ish” -> multiply them into rows
    std::size_t outer = 1;
    for (std::size_t i = 0; i + 1 < rank; ++i)
        outer *= ext[i];

    rows = outer;
}

inline bool isCompactRowMajor(const cords::ViewR &v)
{
    const auto &ext = v.extent();
    const auto rank = ext.rank();
    if (rank == 0)
        return true;

    // Recompute what row-major strides *should* be
    std::size_t expected = 1;
    for (int i = static_cast<int>(rank) - 1; i >= 0; --i) {
        if (v.stride(static_cast<std::size_t>(i)) != expected)
            return false;
        expected *= ext[static_cast<std::size_t>(i)];
    }
    return true;
}


} // namespace job::ai::layers
