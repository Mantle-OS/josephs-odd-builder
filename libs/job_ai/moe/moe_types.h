#pragma once
#include <cstdint>

namespace job::ai::moe {

enum class MoeType : uint8_t {
    Hash = 0,       // Deterministic hashing of input
    TopK,           // Learned gating (standard MoE)
    Spatial,        // "Geology" based (Region A -> Expert A)
    State           // Finite State Machine based
};
} // namespace job::ai::moe

