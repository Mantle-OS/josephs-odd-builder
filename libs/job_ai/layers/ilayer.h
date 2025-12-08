#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstddef>

#include <job_thread_pool.h>

#include "view.h"
#include "extents.h"
#include "workspace.h"
#include "layer_types.h"
#include "iparamgroup.h"

namespace job::ai::layers {

class ILayer {
public:
    using Ptr = std::shared_ptr<ILayer>;

    virtual ~ILayer() = default;

    // What kind of layer is this? (Dense, Attention, etc.)
    [[nodiscard]] virtual LayerType type() const noexcept = 0;

    // Human-readable name: "ffn", "attn.q_proj", "residual_3", etc.
    [[nodiscard]] virtual const std::string &name() noexcept = 0;

    // Given an input shape, what does this layer produce?
    [[nodiscard]] virtual cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const = 0;

    // Forward pass: input/output are non-owning views into contiguous float buffers.
    // Workspace is scratch memory for temporaries.
    virtual void forward(job::threads::ThreadPool &pool,
                         const cords::ViewR       &input,
                         cords::ViewR             &output,
                         infer::Workspace         &workspace) = 0;

    // Flat parameter count (total number of floats this layer owns).
    [[nodiscard]] virtual std::size_t parameterCount() const noexcept = 0;

    // Flat view over all parameters this layer owns (weights + biases + etc.)
    // Non-owning; underlying storage is owned by the model/engine.
    [[nodiscard]] virtual cords::ViewR parameters() noexcept = 0;

    // Optional const view for callers that only need read-only access.
    // [[nodiscard]] virtual cords::ViewConstR parameters() const noexcept = 0;

    // Reset any internal state (KV cache, running stats, RNG state, etc.).
    virtual void resetState() noexcept {};
};

} // namespace job::ai::layers
