#pragma once

#include <memory>

#include <job_stencil_grid_2D.h>

#include "iadapter.h"
#include "adapter_types.h"

#include "stencil_config.h"

namespace job::ai::adapters {

class StencilAdapter : public IAdapter {
public:
    static std::unique_ptr<StencilAdapter> unique(StencilConfig cfg = {})
    {
        return std::make_unique<StencilAdapter>(cfg);
    }
    explicit StencilAdapter(StencilConfig cfg);
    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adaptParallel(job::threads::ThreadPool &pool,
               const cords::AttentionShape &shape,
               const cords::ViewR &sources,
               [[maybe_unused]] const cords::ViewR &targets,
               [[maybe_unused]] const cords::ViewR &values,
               cords::ViewR &output,
               [[maybe_unused]] const AdapterCtx &ctx) override;


    void adapt(job::threads::ThreadPool &pool,
                       const cords::AttentionShape &shape,
                       const cords::ViewR &sources,
                       [[maybe_unused]] const cords::ViewR &targets,
                       [[maybe_unused]] const cords::ViewR &values,
                       cords::ViewR &output,
                       [[maybe_unused]] const AdapterCtx &ctx) override;

private:
    StencilConfig m_cfg;
};

} // namespace job::ai::adapters
