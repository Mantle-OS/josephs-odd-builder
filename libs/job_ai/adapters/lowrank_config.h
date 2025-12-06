#pragma once
namespace job::ai::adapters {
struct LowRankConfig {
    float scale = 1.0f;
    // Could add kernel type (ReLU, ELU+1) here later
};
}
