#pragma once

#include "integrators/iintegrator.h"
#include "solvers/job_barnes_hut_calculator.h"
#include "solvers/job_verlet_integrator.h"
#include "data/forces.h"

namespace job::science::integrators {

using job::science::data::Particle;
using job::science::data::Vec3f;

class BarnesHutStrategy final : public IIntegrator {
public:
    using BHCalc = job::science::BarnesHutForceCalculator<Particle, Vec3f, float>;
    using Verlet = job::science::VerletIntegrator<Particle, Vec3f>;

    explicit BarnesHutStrategy(float theta = 0.5f, float G = 6.674e-11f) :
        m_theta(theta),
        m_G(G)
    {

    }

    engine::IntegratorType type() const noexcept override
    {
        return engine::IntegratorType::BarnesHut;
    }

    void prime(IntegratorCtx &ctx) override {
        bindIfNeeded(ctx);
        m_accelCalc(ctx.particles);
        m_primed = true;
    }

    void step(IntegratorCtx &ctx, float dt) override
    {
        if (dt <= 0.0f)
            return;

        bindIfNeeded(ctx);
        if (!m_primed)
            prime(ctx);

        // We use Verlet KDK scheme for stability, powered by BH forces
        m_verlet->step(dt, job::science::VVScheme::KDK, true);

        onStepCompleted(0.0f);
    }

    void reset() noexcept override
    {
        IIntegrator::reset();
        m_primed = false;
        m_particlesPtr = nullptr;
        m_bhCalc.reset();
        m_verlet.reset();
    }

private:
    void bindIfNeeded(IntegratorCtx &ctx)
    {
        if (m_particlesPtr == &ctx.particles && m_verlet)
            return;

        m_particlesPtr = &ctx.particles;

        m_bhCalc = std::make_unique<BHCalc>(
            ctx.pool,
            [](const Particle& p) -> const Vec3f& { return p.position; },
            [](const Particle& p) -> float { return p.mass; },
            m_theta,
            m_G,
            1e-6f // epsilon_sq softening
            );

        m_accelCalc = [this](std::vector<Particle>& ps)
        {
            if (ps.empty())
                return;

            m_bhCalc->calculate_forces(ps, m_forceBuffer);
            for (size_t i = 0; i < ps.size(); ++i) {
                float m = ps[i].mass;
                if (m > 0.0f)
                    ps[i].acceleration = m_forceBuffer[i] * (1.0f / m);
                else
                    ps[i].acceleration = {0, 0, 0};
            }
        };


        m_verlet = std::make_unique<Verlet>(
            ctx.pool,
            m_particlesPtr,
            [](Particle& p) -> Vec3f& { return p.position; },
            [](Particle& p) -> Vec3f& { return p.velocity; },
            [](Particle& p) -> Vec3f& { return p.acceleration; },
            m_accelCalc
            );
    }

    float m_theta;
    float m_G;
    bool m_primed{false};

    std::vector<Particle> *m_particlesPtr{nullptr};
    std::vector<Vec3f> m_forceBuffer;

    std::unique_ptr<BHCalc> m_bhCalc;
    std::unique_ptr<Verlet> m_verlet;
    Verlet::AccelCalculator m_accelCalc;
};

} // namespace job::science::integrators
