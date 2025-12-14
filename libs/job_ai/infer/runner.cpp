#include "runner.h"
#include <cstring>
#include "layer_factory.h"
#include "noise_table.h"

namespace job::ai::infer {

Runner::Runner(const evo::Genome &genome,
               threads::ThreadPool::Ptr pool,
               uint8_t initialWsMB)
    : m_pool(std::move(pool))
    , m_genome(genome)
    // Reserve minimal space initially
    , m_workspace(size_t(initialWsMB) * 1024 * 1024)
{
    comp::NoiseTable::instance();
    buildNetwork();
}

void Runner::buildNetwork()
{
    m_layers.clear();
    size_t maxDim = 0;

    // Scan genome to find the widest layer (for buffer sizing)
    for (const auto &gene : m_genome.architecture) {
        auto layer = layers::LayerFactory::create(gene, m_genome.weights);
        if (layer)
            m_layers.push_back(std::move(layer));

        size_t layerWidth = gene.inputs;
        // Check output width too
        if (gene.outputs > layerWidth) layerWidth = gene.outputs;

        if (layerWidth > maxDim) maxDim = layerWidth;
    }
    m_maxLayerDim = (maxDim > 0) ? maxDim : 1;
}

void Runner::reset() {
    for(auto &l : m_layers) l->resetState();
}

bool Runner::isCompatible(const evo::Genome &g) const {
    size_t totalParams = 0;
    for(const auto& layer : m_layers)
        totalParams += layer->parameterCount();
    return g.weights.size() == totalParams;
}

void Runner::reload(const evo::Genome &genome) {
    if (isCompatible(genome)) {
        float *rawData = const_cast<float*>(genome.weights.data());
        size_t offset = 0;
        for (auto& layer : m_layers) {
            layer->loadWeights(rawData + offset);
            offset += layer->parameterCount();
        }
        m_genome = genome;
    } else {
        m_genome = genome;
        buildNetwork();
    }
}

cords::ViewR Runner::run(const cords::ViewR &input, uint8_t wsMB)
{
    cords::ViewR::Extent shape = input.extent();
    size_t totalRows = shape[0];
    if (shape.rank() >= 3) totalRows *= shape[1];

    // IO Buffer Size (Floats)
    size_t ioFloats = totalRows * m_maxLayerDim;
    // Align to 16/64 floats for SIMD
    ioFloats = (ioFloats + 15) & ~15;

    size_t scratchBytes = size_t(wsMB) * 1024 * 1024;

    // Total Bytes Needed
    size_t totalBytes = (ioFloats * 2 * sizeof(float)) + scratchBytes;
    if (m_workspace.size() < totalBytes)
        m_workspace.resize(totalBytes); // HERE

    // 3. Partition Pointers
    float *base = m_workspace.raw();
    float *bufA = base;
    float *bufB = base + ioFloats;

    // Scratch starts after Buffer B
    float* scratchBase = base + (ioFloats * 2);

    // Create a View for the layers to use as scratch
    // They won't know bufA/B exist before them.
    Workspace scratchView(scratchBase, scratchBytes);

    // Safe because bufA is part of m_workspace which we own
    if (input.size() <= ioFloats)
        std::memcpy(bufA, input.data(), input.size() * sizeof(float));
    else
        std::memcpy(bufA, input.data(), ioFloats * sizeof(float));


    // Execution Loop
    bool flip = false;
    cords::ViewR::Extent currentShape = shape;

    for (auto &layer : m_layers) {
        float *srcPtr = flip ? bufB : bufA;
        float *dstPtr = flip ? bufA : bufB;

        cords::ViewR src(srcPtr, currentShape);
        cords::ViewR::Extent nextShape = layer->getOutputShape(currentShape);
        cords::ViewR dst(dstPtr, nextShape);

        // Pass the VIEW, so the layer writes to scratchBase, not bufA/B
        layer->forward(*m_pool, src, dst, scratchView);

        flip = !flip;
        currentShape = nextShape;
    }

    float *finalPtr = flip ? bufB : bufA;
    return cords::ViewR(finalPtr, currentShape);
}

} // namespace job::ai::infer
