#pragma once

#include <memory>

#include <job_thread_pool.h>

#include "engine/engine_config.h"

#include "integrator_ctx.h"
#include "iintegrator.h"

#include "integrators/verlet.h"
// #include "integrators/barns_hut.h"
// #include "integrators/fmm.h"


namespace job::science::integrators {

std::unique_ptr<IIntegrator> create(engine::IntegratorType type)
{
    switch (type) {
    case engine::IntegratorType::VelocityVerlet:
        return std::make_unique<VerletStrategy>(true, true);
    case engine::IntegratorType::RungeKutta4:
    case engine::IntegratorType::BarnesHut:
    case engine::IntegratorType::FastMultipoleMethod:
    case engine::IntegratorType::Euler:
        return nullptr;
    default:
        return nullptr;
    }
    return nullptr;
}

}
