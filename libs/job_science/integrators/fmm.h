#pragma once

#include "integrators/iintegrator.h"
#include "solvers/job_fmm_integrator.h"
#include "solvers/job_verlet_integrator.h"
#include "data/particle.h"
#include "data/forces.h"

namespace job::science::integrators {

using job::science::data::Particle;
using job::science::data::Vec3f;

/**
 * FMM Traits to bridge the engine's Particle type
 * to the FMM Solver.
 */
struct FmmStrategyTraits {
    static Vec3f position(const Particle &p) { return p.position; }
    static float mass(const Particle &p) { return p.mass; }
    static void applyForce(Particle &p, const Vec3f &accel) {
        // FMM usually calculates potential or field (acceleration)
        p.acceleration = p.acceleration + accel;
    }
};

class FmmStrategy final : public IIntegrator {
public:
    using FMMEngine = job::science::JobFmmEngine<Particle, Vec3f, float, FmmStrategyTraits>;
    using Verlet = job::science::VerletIntegrator<Particle, Vec3f>;

    explicit FmmStrategy(float theta = 0.6f, int maxLeafSize = 16) {
        m_params.theta = theta;
        m_params.maxLeafSize = maxLeafSize;
    }

    engine::IntegratorType type() const noexcept override {
        return engine::IntegratorType::FastMultipoleMethod;
    }

    void prime(IntegratorCtx &ctx) override {
        bindIfNeeded(ctx);
        m_accelCalc(ctx.particles);
        m_primed = true;
    }

    void step(IntegratorCtx &ctx, float dt) override {
        if (dt <= 0.0f) return;
        bindIfNeeded(ctx);
        if (!m_primed) prime(ctx);

        // Verlet step powered by FMM field calculations
        m_verlet->step(dt, job::science::VVScheme::KDK, true);
        onStepCompleted(0.0f);
    }

    void reset() noexcept override {
        IIntegrator::reset();
        m_primed = false;
        m_particlesPtr = nullptr;
        m_fmmEngine.reset();
        m_verlet.reset();
    }


private:
    void bindIfNeeded(IntegratorCtx &ctx) {
        // Rebind if the data containers or pool changed
        if (m_particlesPtr == &ctx.particles && m_diskPtr == &ctx.disk && m_fmmEngine) {
            return;
        }

        m_particlesPtr = &ctx.particles;
        m_diskPtr = &ctx.disk;

        // Initialize the FMM Engine with the specific pool from this context
        m_fmmEngine = std::make_unique<FMMEngine>(ctx.pool, m_params);

        // Explicit AccelCalculator: This is the heartbeat of the physics
        m_accelCalc = [this, pool = ctx.pool, disk = m_diskPtr](std::vector<Particle>& ps) {
            if (ps.empty() || !pool) return;

            // FmmStrategyTraits::applyForce uses +=, so we MUST zero out here.
            for (auto& p : ps)
                p.acceleration = {0.0f, 0.0f, 0.0f};

            // Compute N-Body Gravitational Field via FMM
            // This populates acceleration using the P=3 Octupole expansion
            m_fmmEngine->compute(ps);

            //Add Background Potential (The Star/Disk)
            // We do this in parallel to keep performance high
            job::threads::parallel_for(*pool, size_t(0), ps.size(), [&](size_t i) {
                auto& p = ps[i];

                // Get the dominant force: The Star
                Vec3f a_star = data::Forces::netAcceleration(p, *disk, p.composition);

                // Final result = Particle-Particle + Star-Particle
                Vec3f total_a = p.acceleration + a_star;
                if (!job::core::isSafeFinite(total_a.x) || !job::core::isSafeFinite(total_a.y)) {
                    p.acceleration = a_star;
                } else {
                    p.acceleration = total_a;
                }
            });
        };

        m_verlet = std::make_unique<Verlet>(
            ctx.pool,
            m_particlesPtr,
            [](Particle& p) -> Vec3f& { return p.position; },
            [](Particle& p) -> Vec3f& { return p.velocity; },
            [](Particle& p) -> Vec3f& { return p.acceleration; },
            m_accelCalc
            );

        m_primed = false;
    }

    FMMEngine::Params m_params;
    bool m_primed{false};
    std::vector<Particle>* m_particlesPtr{nullptr};
    const data::DiskModel* m_diskPtr{nullptr};

    std::unique_ptr<FMMEngine> m_fmmEngine;
    std::unique_ptr<Verlet> m_verlet;
    Verlet::AccelCalculator m_accelCalc;
};

} // namespace job::science::integrators
