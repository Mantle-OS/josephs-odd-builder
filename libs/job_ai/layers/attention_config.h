#pragma once
#include <cstdint>
#include "adapter_types.h"
namespace job::ai::layers {
struct AttentionConfig {
    adapters::AdapterType   adapterType{adapters::AdapterType::Flash};      // The "Engine" (Flash, FMM, etc.)
    uint32_t                numHeads{8};
    bool                    causal{true};                                   // Mask future tokens maybe
    bool                    useBias{false};                                 // Standard LLaMA ignores bias in attention projections
};
} // namespace job::ai::layers
