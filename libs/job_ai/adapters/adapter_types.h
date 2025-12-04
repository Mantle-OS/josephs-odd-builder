#pragma once
#include <cstdint>

namespace job::ai::adapters {

enum class AdapterType : uint8_t {
    None = 0,
    FMM,        // Fast Multipole Method (Gravity/Electrostatics)
    BarnesHut,  // Tree-code (Gravity)
    Verlet,     // Velocity Verlet (Dynamics)
    LowRank,    // LoRA / SVD based approximation
    Kernelized  // Linear Attention kernels
};

enum class Precision : uint8_t {
    Float32,
    Float64, // For high-precision physics
    Mixed    // BF16/FP32 mix
};

} // namespace job::ai::adapters
