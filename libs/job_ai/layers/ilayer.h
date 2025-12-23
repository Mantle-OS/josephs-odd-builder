#pragma once

#include <memory>
#include <string_view>
#include <cstddef>
#include <limits>

#include <job_thread_pool.h>

#include "view.h"

#include "layer_types.h"


namespace job::ai::layers {

class ILayer {
public:
    using Ptr = std::shared_ptr<ILayer>;

    static constexpr float kNoSetAlpha = std::numeric_limits<float>::lowest();

    virtual ~ILayer() = default;

    // What kind of layer is this? (Dense, Attention, etc.)
    [[nodiscard]] virtual LayerType type() const noexcept = 0;

    // Human-readable name: "ffn", "attn.q_proj", "residual_3", etc.
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    // set the layer mode from LayerMode::Training or LayerMode::Inference
    [[nodiscard]] virtual LayerMode layerMode(LayerMode layerMode) = 0;

    // Flat view over all parameters this layer owns (weights + biases + etc.)
    // Non-owning; underlying storage is owned by the model/engine.
    [[nodiscard]] virtual cords::ViewR parameters() noexcept = 0;

    // Flat parameter count (total number of floats this layer owns).
    [[nodiscard]] virtual std::size_t parameterCount() const noexcept = 0;

    // Used(Sometimes) when calling the activation function. This sets (if needed) and returns the new value
    [[nodiscard]] virtual float alpha(float alpha) noexcept = 0;
};

} // namespace job::ai::layers
