#pragma once

#include <cstdint>

namespace job::ai::layers {

enum class LayerType : uint8_t {
    Input = 0,
    Dense,          // Standard Linear
    SparseMoE,      // Mixture of Experts
    Attention,      // Self/Cross Attention
    Embedding,      // Token -> Vector
    LayerNorm,      // Normalization
    RMSNorm,        // LLaMA style norm
    Residual,       // Add
    Output          // Logits
};

// Initializer types for weights
enum class InitType : uint8_t {
    Zero,
    One,
    Uniform,
    Normal,
    Xavier,         // Glorot
    Kaiming,        // He
    Lecun
};

} // namespace job::ai::layers
