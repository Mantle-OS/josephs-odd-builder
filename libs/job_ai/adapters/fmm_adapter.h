#pragma once

#include <memory>

#include <job_fmm_integrator.h>

#include "iadapter.h"
#include "adapter_types.h"

#include "fmm_config.h"

#include <vec3f.h>

namespace job::ai::adapters {

class FmmAdapter final : public IAdapter {
public:
    // From the threads module
    using Solver = threads::JobFmmEngine<FmmTraits::Body, FmmTraits::Vec3,  FmmTraits::Real, FmmTraits>;
    // for the factory
    static std::unique_ptr<FmmAdapter> unique(FmmConfig cfg = {})
    {
        return std::make_unique<FmmAdapter>(cfg);
    }

    explicit FmmAdapter(FmmConfig cfg = {});
    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adaptParallel(
        threads::ThreadPool &pool,
        const cords::AttentionShape &shape,
        const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
        const AdapterCtx &ctx) override;

    void adapt(
        threads::ThreadPool &pool,
        const cords::AttentionShape &shape,
        const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
        const AdapterCtx &ctx) override;

    void apply(threads::ThreadPool &pool, const cords::AttentionShape &shape, const cords::ViewR &sources, cords::ViewR &output,  std::size_t size);



private:
    FmmConfig m_cfg;
};
} // namespace job::ai::adapters
