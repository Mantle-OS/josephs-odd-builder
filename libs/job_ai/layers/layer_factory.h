#pragma once

#include <memory>
#include <vector>
#include <span>

// Core Interfaces
#include "abstract_layer.h"
#include "genome.h"

namespace job::ai::layers {

class LayerFactory {
public:
    static std::unique_ptr<AbstractLayer> create(const evo::LayerGene &gene, const std::vector<float> &genomeWeights);
private:
    static std::unique_ptr<AbstractLayer> createDense(const evo::LayerGene &gene);
    static std::unique_ptr<AbstractLayer> createAttention(const evo::LayerGene &gene);
    static std::unique_ptr<AbstractLayer> createMoE(const evo::LayerGene &gene, const std::vector<float> &genomeWeights);
};

} // namespace job::ai::layers
