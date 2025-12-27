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

void VerletAdapter::adaptParallel(ThreadPool &pool,
                                  const AttentionShape &shape,
                                  const ViewR &sources,
                                  [[maybe_unused]] const ViewR &targets,
                                  [[maybe_unused]] const ViewR &values,
                                  ViewR &output,
                                  const AdapterCtx &ctx
                                  )
{
    const size_t dim = static_cast<size_t>(shape.dim);
    if (dim < 3)
        return;


    std::shared_ptr<ThreadPool> poolPtr(&pool, [](void*){});

    const size_t batch = static_cast<size_t>(shape.batch);
    const size_t seq = static_cast<size_t>(shape.seq);

    threads::parallel_for(pool, size_t{0}, batch, [&](size_t b) {
        apply(seq, dim, poolPtr, sources, output, ctx, b);
    });
}

void VerletAdapter::adapt(ThreadPool &pool,
                          const AttentionShape &shape,
                          const ViewR &sources,
                          [[maybe_unused]] const ViewR &targets,
                          [[maybe_unused]] const ViewR &values, ViewR &output,
                          const AdapterCtx &ctx
                          )
{
    const size_t dim = static_cast<size_t>(shape.dim);
    if (dim < 3)
        return;

    std::shared_ptr<ThreadPool> poolPtr(&pool, [](void*){});

    const size_t batch = static_cast<size_t>(shape.batch);
    const size_t seq = static_cast<size_t>(shape.seq);

    for(size_t i = 0; i < batch; ++i)
        apply(seq, dim, poolPtr, sources, output, ctx, i);
}


void VerletAdapter::apply(size_t seq, size_t dim,
                          threads::ThreadPool::Ptr &pool,
                          const cords::ViewR &sources,
                          cords::ViewR &output,
                          const AdapterCtx &ctx,
                          size_t batchIdx)
{
    const float *in_ptr = sources.data() + (batchIdx * seq * dim);
    float *out_ptr = output.data() + (batchIdx * seq * dim);

    std::memcpy(out_ptr, in_ptr, seq * dim * sizeof(float));

    std::vector<float> accelerations(seq * 3, 0.0f);

    std::vector<float> temp_vel;
    if (dim < kVV_Velocity)
        temp_vel.resize(seq * 3, 0.0f);

    std::vector<MLParticle> particles;
    particles.reserve(seq);

    for (size_t i = 0; i < seq; ++i) {
        size_t idx = i * dim;

        float *pPtr = out_ptr + idx;
        float *vPtr = (dim >= kVV_Velocity) ? (out_ptr + idx + 3) : (&temp_vel[i * 3]);
        float *aPtr = &accelerations[i * 3]; // Point to flat buffer

        float mass = 1.0f;
        if (dim >= kVV_Mass) {
            mass = std::abs(pPtr[6]); // Read from output (already copied)
            if (mass < 1e-4f)
                mass = 1.0f;
        }

        particles.push_back({
            .pos = MLVec3f{ pPtr }, // Points to out_ptr[0,1,2]
            .vel = MLVec3f{ vPtr }, // Points to out_ptr[3,4,5]
            .acc = MLVec3f{ aPtr }, // Points to []
            .mass = mass
        });
    }

    Solver integrator(pool, &particles,
                      [](MLParticle &p) -> MLVec3f& { return p.pos; },
                      [](MLParticle &p) -> MLVec3f& { return p.vel; },
                      [](MLParticle &p) -> MLVec3f& { return p.acc; },
                      [](std::vector<MLParticle> &ps) { computeNbodyForces(ps); }
                      );

    float dt = (ctx.dt > 0.0f) ? ctx.dt : m_cfg.dt;
    integrator.step(dt, science::VVScheme::KDK, false); // this is wrong now
}
} // namespace job::ai::adapters
