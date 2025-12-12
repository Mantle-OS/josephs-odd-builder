#include "runner.h"
#include <cstring>
#include "layer_factory.h"
#include "noise_table.h"

namespace job::ai::infer {

// TODO this code is shity wrote to quick FIXME I really just wanted to test a full loop

#ifndef JOB_DEFAULT_WS_MB
#define JOB_DEFAULT_WS_MB 256 // 256MB Scratchpad
#endif

Runner::Runner(const evo::Genome &genome,
               threads::ThreadPool::Ptr pool,
               size_t workspaceSizeMB):
    m_pool(std::move(pool)),
    m_genome(genome),
    m_workspace(workspaceSizeMB * 1024 * 1024)
{
    comp::NoiseTable::instance();
    buildNetwork();
}

void Runner::buildNetwork()
{
    m_layers.clear();
    size_t maxDim = 0;
    m_maxLayerDim = 0;

    for (const auto &gene : m_genome.architecture) {
        auto layer = layers::LayerFactory::create(gene, m_genome.weights);
        if (layer)
            m_layers.push_back(std::move(layer));

        size_t layerWidth = 0;
        switch (gene.type) {
        case layers::LayerType::Dense:
            layerWidth = std::max(gene.inputs, gene.outputs);
            break;
        case layers::LayerType::Attention:
            layerWidth = gene.inputs;
            break;
        case layers::LayerType::SparseMoE:
            layerWidth = gene.inputs;
            break;
        default:
            layerWidth = gene.inputs;
        }

        if (layerWidth > maxDim)
            maxDim = layerWidth;
    }

    m_maxLayerDim = (maxDim > 0) ? maxDim : 1;
}

void Runner::reset()
{
    for(auto &l : m_layers)
        l->resetState();
}

cords::ViewR Runner::run(const cords::ViewR &input)
{
    cords::ViewR::Extent shape = input.extent();
    // if rank 3 [batch, seq, dim], rows = batch * seq
    // if rank 2 [batch, dim], rows = batch
    size_t totalRows = shape[0];
    if (shape.rank() >= 3)
        totalRows *= shape[1];

    size_t requiredSize = totalRows * m_maxLayerDim;

    if (input.size() > requiredSize)
        requiredSize = input.size();

    if (m_bufA.size() < requiredSize) {
        m_bufA.resize(requiredSize);
        m_bufB.resize(requiredSize);
    }

    float *ptrA = m_bufA.data();
    float *ptrB = m_bufB.data();

    std::memcpy(ptrA, input.data(), input.size() * sizeof(float));

    bool flip = false;
    cords::ViewR::Extent currentShape = shape;

    for (auto& layer : m_layers) {
        float *srcPtr = flip ? ptrB : ptrA;
        float *dstPtr = flip ? ptrA : ptrB;

        cords::ViewR src(srcPtr, currentShape);

        cords::ViewR::Extent nextShape = layer->getOutputShape(currentShape);
        cords::ViewR dst(dstPtr, nextShape);

        layer->forward(*m_pool, src, dst, m_workspace);

        flip = !flip;
        currentShape = nextShape;
    }

    float *finalPtr = flip ? ptrB : ptrA;
    return cords::ViewR(finalPtr, currentShape);
}

} // namespace job::ai::infer
