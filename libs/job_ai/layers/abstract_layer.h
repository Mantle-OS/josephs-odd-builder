#pragma once

#include <string_view>

#include "ilayer.h"
#include "layer_config.h"
#include "workspace.h"

namespace job::ai::layers {

class AbstractLayer : public ILayer {
public:
    using Ptr = std::shared_ptr<AbstractLayer>;
    explicit AbstractLayer(LayerConfig cfg, float alpha = 0.0f) noexcept :
        m_cfg{cfg},
        m_alpha(alpha)
    {
    }

    virtual ~AbstractLayer() = default;
    AbstractLayer(const AbstractLayer&) = delete;
    AbstractLayer &operator=(const AbstractLayer&) = delete;
    AbstractLayer(AbstractLayer&&) noexcept = default;
    AbstractLayer &operator=(AbstractLayer&&) noexcept = default;

    /////////////////////////////////////////
    // Start virtual functions
    /////////////////////////////////////////

    // Forward pass: input/output are non-owning views into contiguous float buffers.
    // Workspace is scratch memory for temporaries.
    virtual void forward(job::threads::ThreadPool &pool,
                         const cords::ViewR       &input,
                         cords::ViewR             &output,
                         infer::Workspace         &workspace) = 0;


    // The layer must consume its parameters from 'weights' and return  the number of floats consumed (or simply assume it consumes parameterCount()).
    // DONT FORGET TO RESET !!!!!!
    virtual void loadWeights(float *weights) = 0 ;

    // the size of the scratch pad.
    [[nodiscard]] virtual size_t scratchSize(const cords::ViewR::Extent &extent) const = 0;

    // Reset any internal state (KV cache, running stats, RNG state, etc.). when I get here ....
    virtual void resetState() noexcept = 0;

    // Given an input shape, what does this layer produce?
    [[nodiscard]] virtual cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const = 0;


    /////////////////////////////////////////
    // Start ILayer
    /////////////////////////////////////////
    [[nodiscard]] virtual LayerType type() const noexcept override
    {
        return LayerType::Abstract;
    }

    [[nodiscard]] std::string_view name() const noexcept override
    {
        const char *p = m_cfg.name.data();
        std::size_t len = 0;
        while (len < m_cfg.name.size() && p[len] != '\0')
            ++len;
        return std::string_view(p, len);
    }

    [[nodiscard]] LayerMode layerMode(LayerMode layerMode) override
    {
        m_cfg.mode = layerMode;
        return m_cfg.mode;
    }

    [[nodiscard]] float alpha(float a) noexcept override
    {
        if (a != ILayer::kNoSetAlpha)
            m_alpha = a;
        return m_alpha;
    }

    // WARNING: Default implementation assumes Standard Dense topology (In * Out [+ Bias]).
    [[nodiscard]] virtual  std::size_t parameterCount() const noexcept override
    {
        size_t total = m_cfg.inputs * m_cfg.outputs;
        if (m_cfg.hasBias())
            total += m_cfg.outputs;
        return total;
    }

protected:
    LayerConfig                                                 m_cfg;
    float                                                       m_alpha{0.0f};
};

} // namespace job::ai::layers
