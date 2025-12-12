#include "rk4_adapter.h"
namespace job::ai::adapters {

using namespace job::ai::cords;
using namespace job::threads;

Rk4Adapter::Rk4Adapter(Rk4Config cfg) :
    m_cfg(cfg)
{

}

AdapterType Rk4Adapter::type() const
{
    return AdapterType::RK4;
}

std::string Rk4Adapter::name() const
{
    return "RK4 (Runge-Kutta 4th Order)";
}

void Rk4Adapter::adapt(
    job::threads::ThreadPool &pool,
    const AttentionShape &shape,
    const ViewR &sources, [[maybe_unused]] const ViewR &targets, [[maybe_unused]] const ViewR &values, ViewR &output,
    const AdapterCtx &ctx
    )
{
    const int B = static_cast<int>(shape.batch);
    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);

    if (D < 3)
        return;

    std::shared_ptr<ThreadPool> poolPtr(&pool, [](void*){});

    job::threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        std::vector<Particle> bodies(S);
        const float *in_ptr = sources.data() + (b * S * D);
        float *out_ptr      = output.data()  + (b * S * D);

        for (int i = 0; i < S; ++i) {
            auto &p = bodies[i];
            int idx = i * D;

            // Map Position
            p.pos.x = in_ptr[idx + m_cfg.dim_mapping[0]];
            p.pos.y = in_ptr[idx + m_cfg.dim_mapping[1]];
            p.pos.z = in_ptr[idx + m_cfg.dim_mapping[2]];

            // Map Velocity (if dims exist)
            if (D >= 6) {
                p.vel.x = in_ptr[idx + m_cfg.dim_mapping[3]];
                p.vel.y = in_ptr[idx + m_cfg.dim_mapping[4]];
                p.vel.z = in_ptr[idx + m_cfg.dim_mapping[5]];
            } else {
                p.vel = {0,0,0};
            }

            // Map Mass
            if (D >= 7) {
                p.mass = std::abs(in_ptr[idx + m_cfg.dim_mapping[6]]);
                if (p.mass < 1e-5f)
                    p.mass = 1.0f;
            } else {
                p.mass = 1.0f;
            }
            p.acc = {0,0,0};
        }

        auto getPos = [](Particle& p) -> Vec3& {
            return p.pos;
        };
        auto getVel = [](Particle& p) -> Vec3& {
            return p.vel;
        };
        auto getAcc = [](Particle& p) -> Vec3& {
            return p.acc;
        };

        auto calcForces = [](std::vector<Particle>& ps) {
            computeNbodyForces(ps);
        };


        JobRK4Integrator<Particle, Vec3, float> integrator(
            poolPtr, &bodies, getPos, getVel, getAcc, calcForces, false
            );

        float dt = (ctx.dt > 0.0f) ? ctx.dt : m_cfg.dt;
        integrator.step_n(m_cfg.steps, dt);

        for (int i = 0; i < S; ++i) {
            int idx = i * D;
            const auto &p = bodies[i];

            out_ptr[idx + m_cfg.dim_mapping[0]] = p.pos.x;
            out_ptr[idx + m_cfg.dim_mapping[1]] = p.pos.y;
            out_ptr[idx + m_cfg.dim_mapping[2]] = p.pos.z;

            if (D >= 6) {
                out_ptr[idx + m_cfg.dim_mapping[3]] = p.vel.x;
                out_ptr[idx + m_cfg.dim_mapping[4]] = p.vel.y;
                out_ptr[idx + m_cfg.dim_mapping[5]] = p.vel.z;
            }

            if (D > 6)
                std::memcpy(out_ptr + idx + 6, in_ptr + idx + 6, (D - 6) * sizeof(float));
        }
    });
}

}
