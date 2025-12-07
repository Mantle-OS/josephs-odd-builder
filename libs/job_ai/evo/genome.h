#pragma once

#include <cstdint>
#include <vector>

#include "activation_types.h"
#include "layer_types.h"

namespace job::ai::evo {

namespace layers = job::ai::layers;
namespace comp   = job::ai::comp;

// Silly math
struct LayerGene {
    layers::LayerType       type{layers::LayerType::Dense};                    // the type of layer
    comp::ActivationType    activation{comp::ActivationType::Identity};        // how this thing is activated
    std::uint8_t            _pad[2]{};                                         // 2 bytes explicit padding (!!!!Zero this out!!!!!)
    std::uint32_t           inputs{0};                                         // number of input neurons/features
    std::uint32_t           outputs{0};                                        // number of output neurons/features
    std::uint32_t           weightOffset{0};                                   // index into Genome::weights
    std::uint32_t           weightCount{0};                                    // how many floats this layer owns
    std::uint32_t           biasOffset{0};                                     // index into Genome::weights
    std::uint32_t           biasCount{0};                                      // how many floats for bias
    std::uint32_t           auxiliaryData{0};                                  // e.g., number of experts or FMM order p
};

static_assert(sizeof(LayerGene) == 32,
              "LayerGene must be 32 bytes for cache alignment");

using LayerGenes = std::vector<LayerGene>;

struct GenomeHeader {
    std::uint64_t           magic{0};                                           // padding and magic number
    std::uint64_t           uuid{0};                                            // UUID
    std::uint64_t           parentId{0};                                        // tracking evolutionary trees
    std::uint32_t           generation{0};                                      // epoch number
    float                   fitness{0.0f};                                      // error rate, distance, etc.
    std::uint32_t           layerCount{0};                                      // how many LayerGenes follow?
    std::uint64_t           weightBlobSize{0};                                  // size in bytes of the weight buffer
    std::uint8_t            _res[8]{};                                          // future proofing (zeroed)
};

struct Genome {
    GenomeHeader            header{};                                           // packed header
    bool                    tested{false};                                      // genome run yet?
    LayerGenes              architecture;                                       // NEAT: may grow; fixed topology: stays static
    std::vector<float>      weights;                                            // weights and biases
};

} // namespace job::ai::evo
