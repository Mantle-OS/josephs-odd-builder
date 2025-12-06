#include "verlet_adapter.h"

#include <vector>
#include <cstring>
#include <cmath>

namespace job::ai::adapters {
using namespace job::ai::cords;
using namespace job::threads;

VerletAdapter::VerletAdapter(VerletConfig cfg) :
    m_cfg(cfg)
{
}

AdapterType VerletAdapter::type() const
{
    return AdapterType::Verlet;
}

std::string VerletAdapter::name() const
{
    return "Velocity Verlet Dynamics";
}

void VerletAdapter::adapt(
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
        float *out_ptr = output.data() + (b * S * D);

        // Dims [0,1,2] -> Pos || Dims [3,4,5] -> Vel (if D >= 6) || Dims [6]     -> Mass (if D >= 7)
        for (int i = 0; i < S; ++i) {
            auto &p = bodies[i];
            int idx = i * D;

            p.pos.x = in_ptr[idx + 0];
            p.pos.y = in_ptr[idx + 1];
            p.pos.z = in_ptr[idx + 2]; // Safe because D >= 3 checked

            // velocity
            if (D >= 6) {
                p.vel.x = in_ptr[idx + 3];
                p.vel.y = in_ptr[idx + 4];
                p.vel.z = in_ptr[idx + 5];
            } else {
                p.vel = {0,0,0};
            }

            // mass
            if (D >= 7) {
                p.mass = std::abs(in_ptr[idx + 6]); // mass must be positive
                if (p.mass < 1e-4f) p.mass = 1.0f;
            } else {
                p.mass = 1.0f;
            }

            p.acc = {0,0,0};
        }

        auto get_pos = [](Particle& p) -> Vec3& {
            return p.pos;
        };
        auto get_vel = [](Particle& p) -> Vec3& {
            return p.vel;
        };
        auto get_acc = [](Particle& p) -> Vec3& {
            return p.acc;
        };
        auto calc_accel = [](std::vector<Particle>& ps) {
            computeNbodyForces(ps);
        };

        using Integrator = job::threads::VerletIntegrator<Particle, Vec3>;
        Integrator integrator(poolPtr, &bodies, get_pos, get_vel, get_acc, calc_accel);
        float dt = (ctx.dt > 0.0f) ?
                       ctx.dt :
                       m_cfg.dt;

        //  kick rocks :P
        integrator.step(dt, job::threads::VVScheme::KDK);

        // write back the UPDATED Position and Velocity
        for (int i = 0; i < S; ++i) {
            int idx = i * D;
            const auto &p = bodies[i];

            out_ptr[idx + 0] = p.pos.x;
            out_ptr[idx + 1] = p.pos.y;
            out_ptr[idx + 2] = p.pos.z;

            if (D >= 6) {
                out_ptr[idx + 3] = p.vel.x;
                out_ptr[idx + 4] = p.vel.y;
                out_ptr[idx + 5] = p.vel.z;
            }

            if (D > 6)
                std::memcpy(out_ptr + idx + 6, in_ptr + idx + 6, (D - 6) * sizeof(float));
        }
    });
}

} // namespace job::ai::adapters
