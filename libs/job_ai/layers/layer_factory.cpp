#include "layer_factory.h"

#include <cstring>

#include <job_logger.h>

#include "dense.h"
#include "attention.h"
#include "sparse_moe.h"


// FIXME THIS CODE IS SHIT FIXME

namespace job::ai::layers {

std::unique_ptr<ILayer> LayerFactory::create(const evo::LayerGene& gene, const std::vector<float> &genomeWeights)
{
    std::unique_ptr<ILayer> layer = nullptr;

    switch (gene.type) {
    case LayerType::Dense:
        layer = createDense(gene);
        break;
    case LayerType::Attention:
        layer = createAttention(gene);
        break;
    case LayerType::SparseMoE:
        layer = createMoE(gene, genomeWeights);
        break;
    default:
        JOB_LOG_ERROR("Factory: Unknown LayerType ID {}", (int)gene.type);
        return nullptr;
    }

    if (!layer) {
        JOB_LOG_ERROR("Factory: Failed to instantiate layer type {}", (int)gene.type);
        return nullptr;
    }

    size_t requiredEnd = gene.weightOffset + gene.weightCount;
    if (requiredEnd > genomeWeights.size()) {
        JOB_LOG_ERROR("Factory: Genome underflow. Layer needs weights [{}..{}) but genome size is {}",
                      gene.weightOffset, requiredEnd, genomeWeights.size());
        return nullptr; // Fail safe
    }

    if (gene.weightCount > 0) {
        cords::ViewR layerParams = layer->parameters();
        if (layerParams.size() <= gene.weightCount) {
            float* dst = const_cast<float*>(layerParams.data());
            const float* src = genomeWeights.data() + gene.weightOffset;

            // MEMCPY: The fastest way to load a neural net. Fo rnow MUHAHAHAAHA
            std::memcpy(dst, src, layerParams.size() * sizeof(float));
        } else {
            JOB_LOG_WARN("Factory: Layer param count mismatch. Gene says {}, Layer wants {}",
                         gene.weightCount, layerParams.size());
        }
    }
    return layer;
}

std::unique_ptr<ILayer> LayerFactory::createDense(const evo::LayerGene& gene)
{
    return std::make_unique<Dense>(
        gene.inputs,
        gene.outputs,
        gene.activation
        );
}

std::unique_ptr<ILayer> LayerFactory::createAttention(const evo::LayerGene& gene)
{
    AttentionConfig cfg;
    cfg.numHeads = gene.outputs;
    cfg.useBias = true;
    cfg.adapterType = static_cast<adapters::AdapterType>(gene.auxiliaryData);

    return std::make_unique<Attention>(cfg, gene.inputs);
}

std::unique_ptr<ILayer> LayerFactory::createMoE(const evo::LayerGene &gene, [[maybe_unused]]const std::vector<float> &genomeWeights)
{
    int dim = gene.inputs;
    int numExperts = gene.outputs;
    int k = gene.auxiliaryData; // Top-K stored in aux

    // Validation
    if (numExperts <= 0 || k <= 0)
        return nullptr;

    auto moe = std::make_unique<moe::SparseMoE>(dim, numExperts, k);
    moe->setRouterType(router::RouterType::TopK);
    for(int i=0; i<numExperts; ++i) {
        auto expert = std::make_unique<Dense>(dim, dim, comp::ActivationType::GELU);
        moe->addExpert(i, std::move(expert));
    }

    return moe;
}

} // namespace job::ai::layers
