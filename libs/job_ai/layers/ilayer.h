#pragma once

#include <vector>
#include <string>
#include <memory>

#include <job_thread_pool.h>

#include "view.h"
#include "layer_types.h"

namespace job::ai::layers {

class ILayer {
public:
    using Ptr = std::shared_ptr<ILayer>;

    virtual ~ILayer() = default;

    [[nodiscard]] virtual LayerType type() const = 0;
    [[nodiscard]] virtual std::string name() const = 0;

    // The Forward Pass Contract
    // enforce non-allocating, zero-copy where possible.
    // The output view must be pre-allocated by the caller (Runner).
    virtual void forward(job::threads::ThreadPool &pool, const cords::ViewR &input, cords::ViewR &output ) = 0;

    // Parameter Access (for Evolution/Training)
    // Returns a flattened view of all trainable weights.
    [[nodiscard]] virtual size_t parameterCount() const = 0;

    // Note: This returns a View, not a copy. Modifying the view modifies the layer.
    [[nodiscard]] virtual cords::ViewR parameters() = 0;

    // State Management (for RNNs/KV-Cache, optional)
    virtual void resetState() {}
};

} // namespace job::ai::layers
