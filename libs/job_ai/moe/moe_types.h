#pragma once
#include <cstdint>

namespace job::ai::moe {

enum class MoeType : uint8_t {
    Hash    = 0,    // Deterministic hashing of input
    TopK    = 1,    // Learned gating (standard MoE)
    Spatial = 2,    // "Geology" based (Region A -> Expert A)
    State   = 3     // Finite State Machine based
};
} // namespace job::ai::moe

