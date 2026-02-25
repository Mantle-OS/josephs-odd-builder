#include "integrator_factory.h"
#include "integrators/verlet.h"
#include "integrators/rk4.h"
#include "integrators/euler.h"
#include "integrators/barns_hut.h"
#include "integrators/fmm.h"

namespace job::science::integrators {

std::unique_ptr<IIntegrator> create(const engine::EngineConfig &cfg)
{
    engine::IntegratorType type = cfg.integrator;
    switch (type) {
    case engine::IntegratorType::VelocityVerlet:
        return std::make_unique<VerletStrategy>(true, true);
    case engine::IntegratorType::RungeKutta4:
        return std::make_unique<RK4Strategy>(true);
    case engine::IntegratorType::BarnesHut:
        return std::make_unique<BarnesHutStrategy>(cfg.barnesHut.theta);
    case engine::IntegratorType::FastMultipoleMethod:
        return std::make_unique<FmmStrategy>(cfg.fmm.theta, cfg.fmm.maxLeafSize);
    case engine::IntegratorType::Euler:
        return std::make_unique<EulerStrategy>();
    default:
        return nullptr;
    }
    return nullptr;
}


};
