#pragma once
#include <cstdint>
namespace job::ai::base {
enum class LayerType : uint8_t {
    Input = 0,      // Where in the hell did you come from
    Dense,          // feed -> forward
    SparseMoE,      // geological router
    AttentionFMM,   // FMM adapter
    AttentionBH,    // barns and hut adapter
    Recurrent,      // RNN/GRU
    Output = 255    // Where in the hell are you going ?
};
}
