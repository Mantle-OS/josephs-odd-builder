#pragma once

// Threading
#include <job_verlet_integrator.h>
#include <job_verlet_adapters.h>

// Science
#include <vec3f.h>

// local
#include "iadapter.h"
#include "adapter_types.h"
#include "verlet_config.h"

namespace job::ai::adapters {
class VerletAdapter : public IAdapter {
public:
    static std::unique_ptr<VerletAdapter> unique(VerletConfig cfg = {})
    {
        return std::make_unique<VerletAdapter>(cfg);
    }
    explicit VerletAdapter(VerletConfig cfg = {});

    [[nodiscard]] AdapterType type() const override;
    [[nodiscard]] std::string name() const override;

    void adapt(job::threads::ThreadPool &pool,
               const cords::AttentionShape &shape,
               const cords::ViewR &sources,  const cords::ViewR &targets,  const cords::ViewR &values,
               cords::ViewR &output,
               const AdapterCtx &ctx) override;

private:
    VerletConfig m_cfg;
};

} // namespace job::ai::adapters
