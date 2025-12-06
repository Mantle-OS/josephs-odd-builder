#pragma once

#include <cstdint>
#include <vector>

#include "view.h"

#include "kv_key.h"

namespace job::ai::kv {

class Cache {
public:
    // Pre-allocate the memory arena.
    // This is critical: We allocate ONCE at startup to avoid malloc during inference.
    void init(uint32_t maxLayers, uint32_t maxHeads, uint32_t maxSeq, uint32_t dim);

    // Store K,V at step t
    // k, v inputs are usually [1, Dim] (the current token's projection)
    void save(const CacheKey &key, uint32_t t,
              const cords::ViewR &k,
              const cords::ViewR &v);

    // Load all past up to t
    // kOut, vOut are views into the internal storage [t+1, Dim]
    // This allows the Attention kernel to read history without copying.
    void load(const CacheKey &key, uint32_t t,
              cords::ViewR &kOut,
              cords::ViewR &vOut);

private:
         // Memory Layout Strategy (Future):
         // 1. Contiguous Slab: [Layers, Heads, MaxSeq, Dim]
         //    - Fast, simple stride math.
         //    - Wastes memory if seq_len is short.
         // 2. PagedAttention (vLLM style): Block tables.
         //    - Complex software MMU.
         //    - Zero fragmentation.

    // For V1, we likely stick to Contiguous or a vector of vectors pre-sized.
};

} // namespace job::ai::kv
