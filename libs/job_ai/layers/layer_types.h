#pragma once

#include "view.h"
#include <cstdint>

namespace job::ai::layers {

enum class LayerType : uint8_t {
    Input       = 0,    // Input data bro...
    Dense       = 1,    // Standard Linear
    SparseMoE   = 2,    // Mixture of Experts
    Attention   = 3,    // Self/Cross Attention
    Embedding   = 4,    // Token -> Vector
    LayerNorm   = 5,    // Normalization
    RMSNorm     = 6,    // LLaMA style norm
    Residual    = 7,    // Add
    LinearLoRA  = 8,    // Meh whatever
    Output      = 9,     // Logits
    Abstract     = 254
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

inline constexpr std::size_t flattenInput(const cords::ViewR &input, std::size_t *rows)
{
    if (input.rank() >= 3) {
        // [Batch, Seq, Dim] -> [Batch*Seq, Dim]
        *rows = input.extent()[0] * input.extent()[1];
        return input.extent()[2];
    }
    else {
        // [Rows, Dim] or [Batch, Dim]
        *rows = input.extent()[0];
        // Handle rank 1 case? [Dim] -> [1, Dim]
        if (input.rank() == 1)
            return input.extent()[0];
        return input.extent()[1];
    }
}



} // namespace job::ai::layers
