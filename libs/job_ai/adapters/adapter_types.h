#pragma once
#include <cstdint>

namespace job::ai::adapters {

enum class AdapterType : uint8_t {
    None = 0,
    Dense,      // "Golden” O(N²) attention using (GEMM + softmax helpers).
    Flash,      // flash_attention_forward
    LowRank,    // LoRA / SVD based approximation
    FMM,        // Fast Multipole Method (Gravity/Electrostatics)
    BarnesHut,  // Tree-code (Gravity)
    Verlet,     // Velocity Verlet (Dynamics)
};

// Kernelized, // Linear Attention kernels
enum class Precision : uint8_t {
    Float32,
    Float64, // For high-precision physics
    Mixed    // BF16/FP32 mix
};

} // namespace job::ai::adapters
