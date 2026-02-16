#pragma once
#include <optional>
#include <memory.h>

#include <job_thread_pool.h>

#include "data/particle.h"
#include "engine/engine_config.h"

#include "data/forces.h"

#include "solvers/job_verlet_adapters.h"
#include "solvers/job_verlet_integrator.h"

#include "integrators/iintegrator.h"

namespace job::science::integrators {
using science::data::Particle;
using science::data::Vec3f;
using science::data::Forces;


class VerletStrategy final : public IIntegrator {
public:
    using Verlet = job::science::VerletIntegrator<Particle, Vec3f>;

    explicit VerletStrategy(bool kdk = true, bool threaded = true)
        : m_kdk(kdk), m_threaded(threaded) {}

    engine::IntegratorType type() const noexcept override {
        return engine::IntegratorType::VelocityVerlet;
    }

    void prime(IntegratorCtx &ctx) override {
        bindIfNeeded(ctx);
        m_accelCalc(ctx.particles);
        m_primed = true;
    }

    void step(IntegratorCtx &ctx, float dt) override {
        if (!(dt > 0.0f))
            return;

        bindIfNeeded(ctx);

        if (!m_primed)
            prime(ctx);

        const auto scheme = m_kdk ? job::science::VVScheme::KDK
                                  : job::science::VVScheme::DKD;

        m_verlet->step(dt, scheme, m_threaded);
    }

    void reset() noexcept override {
        IIntegrator::reset();
        m_primed = false;
        m_particlesPtr = nullptr;
        m_diskPtr = nullptr;
        m_verlet.reset();
        m_accelCalc = {};
    }

private:
    void bindIfNeeded(IntegratorCtx &ctx) {
        // Rebind if any of the backing pointers changed
        const bool diskChanged      = (m_diskPtr      != &ctx.disk);
        const bool particlesChanged = (m_particlesPtr != &ctx.particles);

        if (!diskChanged && !particlesChanged && m_verlet && m_accelCalc)
            return;

        m_diskPtr      = &ctx.disk;
        m_particlesPtr = &ctx.particles;

        // Build acceleration calculator for DISK physics (not SHO)
        m_accelCalc = [pool = ctx.pool, disk = m_diskPtr](data::Particles &ps) {
            const size_t n = ps.size();
            if (!pool || n == 0)
                return;

            job::threads::parallel_for(*pool, size_t(0), n, [&](size_t i) {
                auto &p = ps[i];

                // compute net accel in disk potential
                Vec3f a = Forces::netAcceleration(p, *disk, p.composition);

                // guard fast-math blowups
                if (!job::core::isSafeFinite(a.x) ||
                    !job::core::isSafeFinite(a.y) ||
                    !job::core::isSafeFinite(a.z)) {
                    p.acceleration = {};
                } else {
                    p.acceleration = a;
                }
            });
        };

        // Recreate verlet integrator bound to the new container + accel calc
        m_verlet = std::make_unique<Verlet>(
            ctx.pool,
            m_particlesPtr,
            [](Particle &p)->Vec3f& { return p.position; },
            [](Particle &p)->Vec3f& { return p.velocity; },
            [](Particle &p)->Vec3f& { return p.acceleration; },
            m_accelCalc
            );

        m_primed = false;
    }

    bool                    m_primed{false};
    bool                    m_kdk{true};
    bool                    m_threaded{true};

    data::Particles        *m_particlesPtr{nullptr};
    const data::DiskModel  *m_diskPtr{nullptr};

    Verlet::AccelCalculator m_accelCalc{};
    std::unique_ptr<Verlet> m_verlet;
};
}
