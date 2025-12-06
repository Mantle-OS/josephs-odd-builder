#pragma once
#include <cstdint>
#include "adapter_types.h"
namespace job::ai::layers {
struct AttentionConfig {
    // The "Engine" (Flash, FMM, etc.)
    adapters::AdapterType   adapterType{adapters::AdapterType::Flash};
    uint32_t                numHeads{8};
    bool                    causal{true};      // Mask future tokens maybe
    bool                    useBias{false};    // Standard LLaMA ignores bias in attention projections
};
} // namespace job::ai::layers
