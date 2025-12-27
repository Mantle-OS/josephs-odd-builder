#include "rk4_adapter.h"
#include "ml_particle.h"
// #include "ml_particle.h"

#include <particle.h>
#include <vec3f.h>

namespace job::ai::adapters {

using namespace job::ai::cords;
using namespace job::science::data;
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

void Rk4Adapter::adaptParallel( job::threads::ThreadPool &pool,
                               const AttentionShape &shape,
                               const ViewR &sources, [[maybe_unused]] const ViewR &targets, [[maybe_unused]] const ViewR &values, ViewR &output,
                               const AdapterCtx &ctx)
{
    const size_t batch = static_cast<size_t>(shape.batch);
    const int seq = static_cast<int>(shape.seq);
    const int dim = static_cast<int>(shape.dim);

    // SHIT FIXME
    if (dim < 3)
        return;

    ThreadPool::Ptr poolPtr(&pool, [](void*){});

    job::threads::parallel_for(pool, size_t{0}, size_t(batch), [&](size_t b) {
        apply(seq, dim, poolPtr, sources, output, ctx, b);
    });
}

void Rk4Adapter::adapt(
    job::threads::ThreadPool &pool,
    const AttentionShape &shape,
    const ViewR &sources, [[maybe_unused]] const ViewR &targets, [[maybe_unused]] const ViewR &values, ViewR &output,
    const AdapterCtx &ctx
    )
{
    const size_t batch = static_cast<size_t>(shape.batch);
    const int seq = static_cast<int>(shape.seq);
    const int dim = static_cast<int>(shape.dim);

    // SHIT FIXME
    if (dim < 3)
        return;

    ThreadPool::Ptr poolPtr(&pool, [](void*){});
    for(size_t i = 0 ; i < batch; ++i)
        apply(seq, dim, poolPtr, sources, output, ctx, i);
}



void Rk4Adapter::apply(int seq, int dim,
                       ThreadPool::Ptr pool,
                       const ViewR &sources, ViewR &output,
                       const AdapterCtx &ctx,
                       size_t size
                       )

{
    std::vector<Particle> bodies(seq);
    const float *in_ptr = sources.data() + (size * seq * dim);
    float *out_ptr      = output.data()  + (size * seq * dim);

    for (int i = 0; i < seq; ++i) {
        auto &p = bodies[i];
        int idx = i * dim;

        // Map Position
        p.position[0] = in_ptr[idx + m_cfg.dim_mapping[0]];
        p.position[1] = in_ptr[idx + m_cfg.dim_mapping[1]];
        p.position[2] = in_ptr[idx + m_cfg.dim_mapping[2]];

        // Map Velocity (if dims exist)
        if (dim >= 6) {
            p.position[0] = in_ptr[idx + m_cfg.dim_mapping[3]];
            p.position[1] = in_ptr[idx + m_cfg.dim_mapping[4]];
            p.position[2] = in_ptr[idx + m_cfg.dim_mapping[5]];
        } else {
            p.position = {0,0,0};
        }

        // Map Mass
        if (dim >= 7) {
            p.mass = std::abs(in_ptr[idx + m_cfg.dim_mapping[6]]);
            if (p.mass < 1e-5f)
                p.mass = 1.0f;
        } else {
            p.mass = 1.0f;
        }
        p.acceleration = {0,0,0};
    }


    // auto calcForces = [](std::vector<Particle>& ps) { computeNbodyForces(ps); };

    Solver integrator(pool, &bodies,
                      [](Particle &p) -> Vec3f& { return p.position; },
                      [](Particle &p) -> Vec3f& { return p.velocity; },
                      [](Particle &p) -> Vec3f& { return p.acceleration; },
                      [](std::vector<Particle> &ps) { computeNbodyForces(ps);},
                      false);

    float dt = (ctx.dt > 0.0f) ? ctx.dt : m_cfg.dt;
    integrator.step_n(m_cfg.steps, dt);

    for (int i = 0; i < seq; ++i) {
        int idx = i * dim;
        const auto &p = bodies[i];

        out_ptr[idx + m_cfg.dim_mapping[0]] = p.position.x;
        out_ptr[idx + m_cfg.dim_mapping[1]] = p.position.y;
        out_ptr[idx + m_cfg.dim_mapping[2]] = p.position.z;

        if (dim >= 6) {
            out_ptr[idx + m_cfg.dim_mapping[3]] = p.velocity.x;
            out_ptr[idx + m_cfg.dim_mapping[4]] = p.velocity.y;
            out_ptr[idx + m_cfg.dim_mapping[5]] = p.velocity.z;
        }

        if (dim > 6)
            std::memcpy(out_ptr + idx + 6, in_ptr + idx + 6, (dim - 6) * sizeof(float));
    }

}
}
