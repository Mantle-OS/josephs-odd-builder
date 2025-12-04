#pragma once
#include <cstdint>

namespace job::ai::moe {

enum class RouterType : uint8_t {
    Hash = 0,       // Deterministic hashing of input
    TopK,           // Learned gating (standard MoE)
    Spatial,        // "Geology" based (Region A -> Expert A)
    State           // Finite State Machine based
};

enum class LoadBalanceStrategy : uint8_t {
    None,
    TokenDropping,  // If expert full, drop token
    Overflow,       // Send to shared expert
    AuxLoss         // Penalty for imbalance (training only)
};

} // namespace job::ai::moe

