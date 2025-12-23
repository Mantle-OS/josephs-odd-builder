// kv_cache.h
#pragma once
#include <vector>
#include <cstdint>

namespace job::ai::cache {

struct LayerCache {
    // Layout: [MaxSeq, Heads, HeadDim] (flat buffer)
    // or simplified: [MaxSeq, EmbedDim] per layer if fused
    std::vector<float> k_data;
    std::vector<float> v_data;
};

struct KVCache {
    int max_seq_len = 0;
    int current_pos = 0; // The "Cursor"
    int embed_dim = 0;

    // One cache entry per Attention Layer
    std::vector<LayerCache> layers;

    void resize(int num_layers, int max_seq, int dim) {
        layers.resize(num_layers);
        max_seq_len = max_seq;
        embed_dim = dim;
        current_pos = 0;

        for(auto& l : layers) {
            l.k_data.resize(size_t(max_seq) * dim, 0.0f);
            l.v_data.resize(size_t(max_seq) * dim, 0.0f);
        }
    }

    void reset() {
        current_pos = 0;
    }
};

} // namespace
