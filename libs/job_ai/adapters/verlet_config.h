#pragma once
#include <cstdint>

namespace job::ai::adapters {

inline constexpr uint8_t kVV_Velocity = 6;
inline constexpr uint8_t kVV_Mass = 7;

struct VerletConfig {
    float dt = 0.01f;
    // Add specific force constants here if needed
};

} // namespace job::ai::adapters
