#pragma once
#include <cstdint>
namespace job::ai::adapters {
enum class AdapterType : uint8_t {
    None        = 0,    // Meh whatever
    Dense       = 1,    // "Golden” O(N²) attention using (GEMM + softmax helpers).
    Flash       = 2,    // flash_attention_forward
    LowRank     = 3,    // LoRA / SVD based approximation
    FMM         = 4,    // Fast Multipole Method (Gravity/Electrostatics)
    BarnesHut   = 5,    // Tree-code (Gravity)
    Verlet      = 6,    // Velocity Verlet (Dynamics)
    Stencil     = 7,    // Stencil 2d grid
    RK4         = 8     // Runge-Kutta 4th Order
};

// Kernelized, // Linear Attention kernels
enum class AdapterPrecision : uint8_t {
    Float32     = 0,    // everything else
    Float64     = 1,    // For high-precision physics
    Mixed       = 2     // BF16/FP32 mix
};

} // namespace job::ai::adapters
