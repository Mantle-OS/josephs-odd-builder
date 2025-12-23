#pragma once

#include <cstdint>
#include <vector>
#include <fstream>

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


struct GenomeSerializer {
    static bool save(const Genome& g, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;

        // 1. Header Magic
        const uint32_t magic = 0x47454E4F; // "GENO"
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

        // 2. Architecture
        uint32_t layerCount = static_cast<uint32_t>(g.architecture.size());
        file.write(reinterpret_cast<const char*>(&layerCount), sizeof(layerCount));

        for (const auto& gene : g.architecture) {
            // Explicitly write fields to avoid struct padding/layout issues
            uint32_t type = static_cast<uint32_t>(gene.type);
            uint32_t act  = static_cast<uint32_t>(gene.activation);

            file.write(reinterpret_cast<const char*>(&type), sizeof(type));
            file.write(reinterpret_cast<const char*>(&act), sizeof(act));
            file.write(reinterpret_cast<const char*>(&gene.inputs), sizeof(gene.inputs));
            file.write(reinterpret_cast<const char*>(&gene.outputs), sizeof(gene.outputs));
            file.write(reinterpret_cast<const char*>(&gene.weightCount), sizeof(gene.weightCount));
            file.write(reinterpret_cast<const char*>(&gene.weightOffset), sizeof(gene.weightOffset));
            file.write(reinterpret_cast<const char*>(&gene.auxiliaryData), sizeof(gene.auxiliaryData));
        }

        // 3. Weights
        uint32_t weightCount = static_cast<uint32_t>(g.weights.size());
        file.write(reinterpret_cast<const char*>(&weightCount), sizeof(weightCount));
        if (weightCount > 0) {
            file.write(reinterpret_cast<const char*>(g.weights.data()), weightCount * sizeof(float));
        }

        return file.good();
    }

    static Genome load(const std::string& filename) {
        Genome g;
        std::ifstream file(filename, std::ios::binary);
        if (!file) return g;

        uint32_t magic = 0;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x47454E4F) return g; // Invalid magic

        uint32_t layerCount = 0;
        file.read(reinterpret_cast<char*>(&layerCount), sizeof(layerCount));
        g.architecture.reserve(layerCount);

        for (uint32_t i = 0; i < layerCount; ++i) {
            LayerGene gene{};
            uint32_t type = 0, act = 0;

            file.read(reinterpret_cast<char*>(&type), sizeof(type));
            file.read(reinterpret_cast<char*>(&act), sizeof(act));
            file.read(reinterpret_cast<char*>(&gene.inputs), sizeof(gene.inputs));
            file.read(reinterpret_cast<char*>(&gene.outputs), sizeof(gene.outputs));
            file.read(reinterpret_cast<char*>(&gene.weightCount), sizeof(gene.weightCount));
            file.read(reinterpret_cast<char*>(&gene.weightOffset), sizeof(gene.weightOffset));
            file.read(reinterpret_cast<char*>(&gene.auxiliaryData), sizeof(gene.auxiliaryData));

            gene.type = static_cast<layers::LayerType>(type);
            gene.activation = static_cast<comp::ActivationType>(act);
            g.architecture.push_back(gene);
        }

        uint32_t weightCount = 0;
        file.read(reinterpret_cast<char*>(&weightCount), sizeof(weightCount));
        g.weights.resize(weightCount);
        if (weightCount > 0) {
            file.read(reinterpret_cast<char*>(g.weights.data()), weightCount * sizeof(float));
        }

        return g;
    }
};


} // namespace job::ai::evo
