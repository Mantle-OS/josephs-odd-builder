#include "verlet_adapter.h"
#include <vector>
#include <cstring>
#include <cmath>
#include <data/particle.h>
namespace job::ai::adapters {
using namespace job::ai::cords;
using namespace job::threads;
using namespace job::science::data;

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

void VerletAdapter::adaptParallel(
    ThreadPool &pool,
    const AttentionShape &shape,
    const ViewR &sources,
    [[maybe_unused]] const ViewR &targets,
    [[maybe_unused]] const ViewR &values,
    ViewR &output,
    const AdapterCtx &ctx
    )
{
    const int B = static_cast<int>(shape.batch);
    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);
    if (D < 3)
        return;
    std::shared_ptr<ThreadPool> poolPtr(&pool, [](void*){});

    threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        apply(S, D, poolPtr, sources, output, ctx, b);
    });
}

void VerletAdapter::adapt(
    ThreadPool &pool,
    const AttentionShape &shape,
    const ViewR &sources,
    [[maybe_unused]] const ViewR &targets,
    [[maybe_unused]] const ViewR &values, ViewR &output,
    const AdapterCtx &ctx
    )
{
    const int B = static_cast<int>(shape.batch);
    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);
    if (D < 3)
        return;
    std::shared_ptr<ThreadPool> poolPtr(&pool, [](void*){});

    for(size_t i = 0; i <= size_t(B); ++i)
        apply(S, D, poolPtr, sources, output, ctx, i);
}


void VerletAdapter::apply(int S, int D,
                          threads::ThreadPool::Ptr &pool,
                          const cords::ViewR &sources,
                          cords::ViewR &output,
                          const AdapterCtx &ctx,
                          size_t size)
{

    std::vector<Particle> bodies(S);
    const float *in_ptr = sources.data() + (size * S * D);
    float *out_ptr = output.data() + (size * S * D);

    PhysicsContext physCtx;
    physCtx.particles = bodies.data();
    physCtx.count = S;
    physCtx.valueIn = in_ptr;   // Read V from here
    physCtx.valueOut = out_ptr; // Write Mixed V here
    physCtx.stride = D;
    physCtx.valueDim = (D > 7) ? (D - 7) : 0;

    // Dims [0,1,2] -> Pos || Dims [3,4,5] -> Vel (if D >= 6) || Dims [6]     -> Mass (if D >= 7)
    for (int i = 0; i < S; ++i) {
        auto &p = bodies[i];
        int idx = i * D;

        p.position.x = in_ptr[idx + 0];
        p.position.y = in_ptr[idx + 1];
        p.position.z = in_ptr[idx + 2]; // Safe because D >= 3 checked

        // velocity
        if (D >= 6) {
            p.velocity.x = in_ptr[idx + 3];
            p.velocity.y = in_ptr[idx + 4];
            p.velocity.z = in_ptr[idx + 5];
        } else {
            p.velocity = {0,0,0};
        }
        // mass
        if (D >= 7) {
            p.mass = std::abs(in_ptr[idx + 6]); // mass must be positive
            if (p.mass < 1e-4f)
                p.mass = 1.0f;
        } else {
            p.mass = 1.0f;
        }

        p.acceleration = {0,0,0};
    }

    auto get_pos = [](Particle& p) -> Vec3f& {
        return p.position;
    };
    auto get_vel = [](Particle& p) -> Vec3f& {
        return p.velocity;
    };
    auto get_acc = [](Particle& p) -> Vec3f& {
        return p.acceleration;
    };
    // auto calc_accel = [](std::vector<Particle>& ps) {
    //     computeNbodyForces(ps);
    // };

    auto calc_accel = [&](std::vector<Particle>& /*ignored*/) {
        computeForcesAndMixing(physCtx);
    };


    // the magic !
    science::VerletIntegrator<Particle, Vec3f> integrator(pool, &bodies, get_pos, get_vel, get_acc, calc_accel);
    float dt = (ctx.dt > 0.0f) ?
                   ctx.dt :
                   m_cfg.dt;

    //  kick rocks :P
    integrator.step(dt, science::VVScheme::KDK);

    // write back the UPDATED Position and Velocity
    for (int i = 0; i < S; ++i) {
        int idx = i * D;
        const auto &p = bodies[i];

        out_ptr[idx + 0] = p.position.x;
        out_ptr[idx + 1] = p.position.y;
        out_ptr[idx + 2] = p.position.z;

        if (D >= 6) {
            out_ptr[idx + 3] = p.velocity.x;
            out_ptr[idx + 4] = p.velocity.y;
            out_ptr[idx + 5] = p.velocity.z;
        }

        // if (D > 6)
        //     std::memcpy(out_ptr + idx + 6, in_ptr + idx + 6, (D - 6) * sizeof(float));

        if (D >= 7) {
            out_ptr[idx + 6] = p.mass;
        }
    }
}



} // namespace job::ai::adapters
