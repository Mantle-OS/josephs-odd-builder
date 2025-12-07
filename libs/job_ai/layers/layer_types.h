#pragma once

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
    Output      = 8     // Logits
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

} // namespace job::ai::layers
