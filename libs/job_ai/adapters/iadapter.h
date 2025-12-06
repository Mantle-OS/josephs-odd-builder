#pragma once

#include <job_thread_pool.h>

#include "adapter_types.h"
#include "adapter_ctx.h"
#include "attention_shape.h"
#include "view.h"

namespace job::ai::adapters {
class IAdapter {
public:
    virtual ~IAdapter() = default;
    [[nodiscard]] virtual AdapterType type() const = 0;
    [[nodiscard]] virtual std::string name() const = 0;
    virtual void adapt(job::threads::ThreadPool &pool,
                       const cords::AttentionShape &shape,
                       const cords::ViewR &sources,
                       const cords::ViewR &targets,
                       const cords::ViewR &values,
                       cords::ViewR &output,
                       [[maybe_unused]] const AdapterCtx &ctx) = 0;
};
} // namespace job::ai::adapters

