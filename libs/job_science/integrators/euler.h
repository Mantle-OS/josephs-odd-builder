#pragma once

#include "integrators/iintegrator.h"
#include "solvers/job_euler_integrator.h"
#include "data/forces.h"

namespace job::science::integrators {

using job::science::data::Particle;
using job::science::data::Vec3f;
using job::science::data::Forces;


class EulerStrategy final : public IIntegrator {
public:
    using Euler = job::science::JobEulerIntegrator<Particle, Vec3f, float>;

    explicit EulerStrategy() = default;

    engine::IntegratorType type() const noexcept override {
        return engine::IntegratorType::Euler;
    }

    void prime(IntegratorCtx &ctx) override {
        bindIfNeeded(ctx);
        if (m_accelCalc) {
            m_accelCalc(ctx.particles);
        }
        m_primed = true;
    }

    void step(IntegratorCtx &ctx, float dt) override {
        if (!(dt > 0.0f)) return;

        bindIfNeeded(ctx);
        if (!m_primed) prime(ctx);

        m_euler->step(dt);

        onStepCompleted(0.0f);
    }

    void reset() noexcept override {
        IIntegrator::reset();
        m_primed = false;
        m_particlesPtr = nullptr;
        m_diskPtr = nullptr;
        m_euler.reset();
        m_accelCalc = {};
    }

private:
    void bindIfNeeded(IntegratorCtx &ctx) {
        if (m_particlesPtr == &ctx.particles && m_diskPtr == &ctx.disk && m_euler)
            return;


        m_particlesPtr = &ctx.particles;
        m_diskPtr = &ctx.disk;

        // Standard acceleration calculator for the disk potential
        m_accelCalc = [pool = ctx.pool, disk = m_diskPtr](std::vector<Particle> &ps) {
            if (ps.empty())
                return;

            auto kernel = [&](size_t i) {
                auto &p = ps[i];
                Vec3f a = Forces::netAcceleration(p, *disk, p.composition);

                if (!job::core::isSafeFinite(a.x) || !job::core::isSafeFinite(a.y) || !job::core::isSafeFinite(a.z))
                    p.acceleration = {};
                else
                    p.acceleration = a;
            };

            if (pool) {
                job::threads::parallel_for(*pool, size_t(0), ps.size(), kernel);
            } else {
                for (size_t i = 0; i < ps.size(); ++i)
                    kernel(i);
            }
        };

        // Rebuild the actual solver instance
        m_euler = std::make_unique<Euler>(
            ctx.pool,
            m_particlesPtr,
            [](Particle &p) -> Vec3f& { return p.position; },
            [](Particle &p) -> Vec3f& { return p.velocity; },
            [](Particle &p) -> Vec3f& { return p.acceleration; },
            m_accelCalc
            );

        m_primed = false;
    }

    bool m_primed{false};
    data::Particles* m_particlesPtr{nullptr};
    const data::DiskModel* m_diskPtr{nullptr};

    Euler::AccelCalculator m_accelCalc;
    std::unique_ptr<Euler> m_euler;
};

} // namespace job::science::integrators
