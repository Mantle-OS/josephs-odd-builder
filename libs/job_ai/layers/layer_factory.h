#pragma once

#include <memory>
#include <vector>
#include <span>

// Core Interfaces
#include "ilayer.h"
#include "genome.h"

namespace job::ai::layers {

class LayerFactory {
public:
    static std::unique_ptr<ILayer> create(const evo::LayerGene &gene, const std::vector<float> &genomeWeights);
private:
    static std::unique_ptr<ILayer> createDense(const evo::LayerGene &gene);
    static std::unique_ptr<ILayer> createAttention(const evo::LayerGene &gene);
    static std::unique_ptr<ILayer> createMoE(const evo::LayerGene &gene, const std::vector<float> &genomeWeights);
};

} // namespace job::ai::layers
