#pragma once

#include "integrators/iintegrator.h"
#include "solvers/job_rk4_integrator.h"
#include "data/forces.h"

namespace job::science::integrators {

using job::science::data::Particle;
using job::science::data::Vec3f;
using job::science::data::Forces;

class RK4Strategy final : public IIntegrator {
public:
    using RK4 = job::science::JobRK4Integrator<Particle, Vec3f, float>;

    explicit RK4Strategy(bool threaded = true) :
        m_threaded(threaded)
    {

    }

    engine::IntegratorType type() const noexcept override
    {
        return engine::IntegratorType::RungeKutta4;
    }

    void prime(IntegratorCtx &ctx) override
    {
        bindIfNeeded(ctx);
        if (m_accelCalc)
            m_accelCalc(ctx.particles);
        m_primed = true;
    }

    void step(IntegratorCtx &ctx, float dt) override
    {
        if (!(dt > 0.0f))
            return;

        bindIfNeeded(ctx);
        if (!m_primed)
            prime(ctx);

        m_rk4->step(dt);

        onStepCompleted(0.0f); // timer logic later
    }

    void reset() noexcept override
    {
        IIntegrator::reset();
        m_primed = false;
        m_particlesPtr = nullptr;
        m_diskPtr = nullptr;
        m_rk4.reset();
        m_accelCalc = {};
    }

private:
    void bindIfNeeded(IntegratorCtx &ctx)
    {
        if (m_particlesPtr == &ctx.particles && m_diskPtr == &ctx.disk && m_rk4)
            return;


        m_particlesPtr = &ctx.particles;
        m_diskPtr = &ctx.disk;

        // Force to Acceleration Adapter for the Disk Model
        m_accelCalc = [pool = ctx.pool, disk = m_diskPtr](std::vector<Particle> &ps) {
            if (!pool || ps.empty())
                return;

            job::threads::parallel_for(*pool, size_t(0), ps.size(), [&](size_t i) {
                auto &p = ps[i];
                Vec3f a = Forces::netAcceleration(p, *disk, p.composition);

                // Guard against numerical instability
                if (!job::core::isSafeFinite(a.x) || !job::core::isSafeFinite(a.y) || !job::core::isSafeFinite(a.z))
                    p.acceleration = {0, 0, 0};
                else
                    p.acceleration = a;
            });
        };

        m_rk4 = std::make_unique<RK4>(
            ctx.pool,
            m_particlesPtr,
            [](Particle &p) -> Vec3f& { return p.position; },
            [](Particle &p) -> Vec3f& { return p.velocity; },
            [](Particle &p) -> Vec3f& { return p.acceleration; },
            m_accelCalc,
            m_threaded
            );

        m_primed = false;
    }

    bool m_primed{false};
    bool m_threaded{true};
    std::vector<Particle>* m_particlesPtr{nullptr};
    const data::DiskModel* m_diskPtr{nullptr};

    RK4::AccelCalculator m_accelCalc;
    std::unique_ptr<RK4> m_rk4;
};

} // namespace job::science::integrators
