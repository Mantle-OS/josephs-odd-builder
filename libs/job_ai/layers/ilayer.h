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

    virtual void forward(job::threads::ThreadPool &pool, const cords::ViewR &input, cords::ViewR &output ) = 0;

    [[nodiscard]] virtual size_t parameterCount() const = 0;

    [[nodiscard]] virtual cords::ViewR parameters() = 0;

    virtual void resetState() {}
};

} // namespace job::ai::layers
