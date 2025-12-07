#pragma once
#include <cstdint>
#include <string_view>
namespace job::ai::router {

enum class RouterType : uint8_t {
    Hash    = 0,    // Deterministic hashing of input
    TopK    = 1,    // Learned gating (standard MoE)
    Spatial = 2,    // "Geology" based (Region A -> Expert A)
    State   = 3     // Finite State Machine based
};

[[nodiscard]] inline constexpr std::string_view routerName(RouterType type) noexcept
{
    switch (type) {
    case RouterType::Hash:
        return "Hash – deterministic hash-based router";
    case RouterType::TopK:
        return "TopK – learned softmax gate with top-k experts";
    case RouterType::Spatial:
        return "Spatial – region/coordinate-based router";
    case RouterType::State:
        return "State – finite-state / rule-based router";
    }
    // Safety net in case of corrupted enum
    return "Unknown RouterType";
}


enum class LoadBalanceStrategy : uint8_t {
    None            = 0,    // Whatever ...
    TokenDropping   = 1,    // If expert full, drop token
    Overflow        = 2,    // Send to shared expert
    AuxLoss         = 3     // Penalty for imbalance (training only)
};

[[nodiscard]] inline constexpr std::string_view loadBalanceName(LoadBalanceStrategy lb) noexcept
{
    switch (lb) {
    case LoadBalanceStrategy::None:
        return "None – no explicit load balancing";
    case LoadBalanceStrategy::TokenDropping:
        return "TokenDropping – drop tokens when expert is full";
    case LoadBalanceStrategy::Overflow:
        return "Overflow – spill extra tokens to a backup expert";
    case LoadBalanceStrategy::AuxLoss:
        return "AuxLoss – auxiliary loss to encourage balanced routing";
    }
    return "Unknown LoadBalanceStrategy";
}

} // namespace job::ai::router
