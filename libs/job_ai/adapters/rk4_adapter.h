#pragma once

#include <memory>
#include <string>

#include <job_rk4_integrator.h>

#include <vec3f.h>
#include <particle.h>

#include "iadapter.h"
#include "adapter_types.h"
#include "rk4_config.h"

namespace job::ai::adapters {

class Rk4Adapter : public IAdapter {
public:
    using Solver = science::JobRK4Integrator<science::data::Particle, science::data::Vec3f, float>;
    static std::unique_ptr<Rk4Adapter> unique(Rk4Config cfg = {})
    {
        return std::make_unique<Rk4Adapter>(cfg);
    }

    explicit Rk4Adapter(Rk4Config cfg);

    [[nodiscard]] AdapterType type() const override;

    [[nodiscard]] std::string name() const override;

    void adaptParallel(job::threads::ThreadPool &pool,
                       const cords::AttentionShape &shape,
                       const cords::ViewR &sources,
                       const cords::ViewR &targets,
                       const cords::ViewR &values,
                       cords::ViewR &output,
                       const AdapterCtx &ctx) override;

    void adapt(job::threads::ThreadPool &pool,
                       const cords::AttentionShape &shape,
                       const cords::ViewR &sources,
                       const cords::ViewR &targets,
                       const cords::ViewR &values,
                       cords::ViewR &output,
                       const AdapterCtx &ctx) override;

    void apply(int seq, int dim,
               threads::ThreadPool::Ptr pool,
               const cords::ViewR &sources, cords::ViewR &output,
               const AdapterCtx &ctx,
               size_t size);
private:
    Rk4Config m_cfg;
};

} // namespace job::ai::adapters
