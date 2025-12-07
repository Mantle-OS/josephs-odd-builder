#include "runner.h"
#include <cstring>
#include "layer_factory.h"
#include "noise_table.h"

namespace job::ai::infer {

#ifndef JOB_DEFAULT_WS_MB
#define JOB_DEFAULT_WS_MB 256 // 256MB Scratchpad
#endif

Runner::Runner(const evo::Genome &genome, threads::ThreadPool::Ptr pool):
    m_pool(std::move(pool)),
    m_genome(genome),
    m_workspace(JOB_DEFAULT_WS_MB * 1024 * 1024)
{
    comp::NoiseTable::instance();
    buildNetwork();
}

void Runner::buildNetwork()
{
    m_layers.clear();

    // CRITICAL: We must scan the architecture to find the widest layer.
    // If input is 64 but layer 1 outputs 128, we need buffer space for 128.
    size_t maxDim = 0;
    m_maxLayerDim = 0;

    for (const auto &gene : m_genome.architecture) {

        // 1. Create Layer
        auto layer = layers::LayerFactory::create(gene, m_genome.weights);
        if (layer) {
            m_layers.push_back(std::move(layer));
        }

        // 2. Calculate Width
        size_t layerWidth = 0;
        switch (gene.type) {
        case layers::LayerType::Dense:
            // Dense requires max(Input, Output) because it does in-place bias/act
            // or simply produces larger output.
            layerWidth = std::max(gene.inputs, gene.outputs);
            break;
        case layers::LayerType::Attention:
            // Attention usually preserves embedding dimension (gene.inputs)
            layerWidth = gene.inputs;
            break;
        case layers::LayerType::SparseMoE:
            // MoE usually preserves embedding dimension
            layerWidth = gene.inputs;
            break;
        default:
            layerWidth = gene.inputs;
        }

        if (layerWidth > maxDim) maxDim = layerWidth;
    }

    // Store this for run()
    // We add a safety margin or ensure it's at least 1
    m_maxLayerDim = (maxDim > 0) ? maxDim : 1;
}

void Runner::reset() {
    for(auto& l : m_layers)
        l->resetState();
}



cords::ViewR Runner::run(const cords::ViewR &input)
{
    cords::ViewR::Extent shape = input.extent();

    // CRITICAL FIX: Calculate total "Rows" (Tokens)
    // If Rank 3 [Batch, Seq, Dim], rows = Batch * Seq
    // If Rank 2 [Batch, Dim], rows = Batch
    size_t totalRows = shape[0];
    if (shape.rank() >= 3) {
        totalRows *= shape[1];
    }

    // Calculate worst-case buffer size needed
    // Max Width (128) * Total Rows (4*16=64) = 8192
    size_t requiredSize = totalRows * m_maxLayerDim;

    // Safety: ensure it fits the input itself
    if (input.size() > requiredSize)
        requiredSize = input.size();

    // 1. Resize Buffers (Lazy)
    if (m_bufA.size() < requiredSize) {
        m_bufA.resize(requiredSize);
        m_bufB.resize(requiredSize);
    }

    // ... (The rest of the function remains identical) ...
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


// cords::ViewR Runner::run(const cords::ViewR &input)
// {
//     cords::ViewR::Extent shape = input.extent();

//     // Batch Size (Rows) comes from runtime input.
//     // Feature Size (Cols) comes from our m_maxLayerDim calculation.
//     size_t batchRows = shape[0];

//     // Calculate worst-case buffer size needed
//     size_t requiredSize = batchRows * m_maxLayerDim;

//     // Also ensure it fits the input itself (in case input is the widest thing)
//     if (input.size() > requiredSize)
//         requiredSize = input.size();

//     // 1. Resize Buffers (Lazy)
//     if (m_bufA.size() < requiredSize) {
//         m_bufA.resize(requiredSize);
//         m_bufB.resize(requiredSize);
//     }

//     float *ptrA = m_bufA.data(); // 0  << ODD that it is 0
//     float *ptrB = m_bufB.data(); // 0

//     // 2. Copy Input
//     std::memcpy(ptrA, input.data(), input.size() * sizeof(float));

//     bool flip = false;
//     cords::ViewR::Extent currentShape = shape;

//     // 3. Run Layers
//     for (auto &layer : m_layers) {
//         float *srcPtr = flip ? ptrB : ptrA;
//         float *dstPtr = flip ? ptrA : ptrB;

//         cords::ViewR src(srcPtr, currentShape);

//         // Ask layer what the output shape will be
//         cords::ViewR::Extent nextShape = layer->getOutputShape(currentShape);
//         cords::ViewR dst(dstPtr, nextShape);

//         // Execute
//         layer->forward(*m_pool, src, dst, m_workspace);

//         flip = !flip;
//         currentShape = nextShape;
//     }

//     float *finalPtr = flip ? ptrB : ptrA;
//     return cords::ViewR(finalPtr, currentShape);
// }

} // namespace job::ai::infer
