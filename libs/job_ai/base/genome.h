#pragma once

#include <vector>
#include <cstdint>

#include <real_type.h>

#include "activation_types.h"
#include "layer_types.h"

namespace job::ai::base {

// alignas(64) ??

struct LayerGene {
    LayerType       type;                       // the types of Layer
    ActivationType  activation;                 // how was this thing activated
    uint8_t         _pad[2];                    // 2 bytes explicit padding (!!!!Zero this out!!!!!)
    uint32_t        inputs;                     // number of input neurons/features
    uint32_t        outputs;                    // number of output neurons/features
    uint32_t        weight_offset;              // index into genome::weights
    uint32_t        weight_count;               // how many floats this layer owns
    uint32_t        bias_offset;                // index into genome::weights
    uint32_t        bias_count;                 // how many floats for bias
    uint32_t        auxiliary_data;             // e.g., number of experts or fmm order p
};
static_assert(sizeof(LayerGene) == 32, "LayerGene must be 32 bytes for cache alignment");

using LayerGenes = std::vector<LayerGene>;

struct GenomeHeader {
    uint64_t                magic;              // padding andf magic number
    uint64_t                uuid;               // UUID
    uint64_t                parent_id;          // tracking evolutionary trees
    uint32_t                generation;         // epoch number
    float                   fitness;            // error rate, Distance, etc
    uint32_t                layer_count;        // How many LayerGenes follow?
    uint64_t                weight_blob_size;   // Size in bytes of the weight buffer
    uint8_t                 _res[8];            // Future proofing
};

struct Genome {
    GenomeHeader            header{};           // packed header
    bool                    tested{false};      // genome run yet?
    std::vector<LayerGene>  architecture;       // NEAT, this might grow. In fixed topology, it stays static.... I think we will see
    std::vector<float>      weights;            // weights and biases
};

} // namespace job::ai::base
