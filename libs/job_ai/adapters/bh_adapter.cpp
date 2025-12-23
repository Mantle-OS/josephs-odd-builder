#include "bh_adapter.h"

#include <vector>
#include <cstring>

namespace job::ai::adapters {

using namespace job::ai::cords;
using namespace job::threads;

BhAdapter::BhAdapter(BhConfig cfg) :
    m_cfg(cfg)
{
}

AdapterType BhAdapter::type() const
{
    return AdapterType::BarnesHut;
}

std::string BhAdapter::name() const
{
    return "Barnes-Hut (Tree Code)";
}

void BhAdapter::adaptParallel(
    job::threads::ThreadPool &pool,
    const AttentionShape &shape,
    const ViewR &sources,
    [[maybe_unused]] const ViewR &targets,
    [[maybe_unused]] const ViewR &values,
    ViewR &output,
    [[maybe_unused]] const AdapterCtx &ctx
    )
{
    const size_t B = static_cast<size_t>(shape.batch);
    job::threads::parallel_for(pool, size_t{0}, B, [&](size_t i) {
        apply(pool, shape, sources, output, i);
    });
}

void BhAdapter::adapt(threads::ThreadPool &pool,
                      const cords::AttentionShape &shape,
                      const cords::ViewR &sources,
                      [[maybe_unused]] const cords::ViewR &targets,
                      [[maybe_unused]] const cords::ViewR &values,
                      cords::ViewR &output,
                      [[maybe_unused]]const AdapterCtx &ctx)
{
    const size_t B = static_cast<size_t>(shape.batch);
    for(size_t i = 0 ; i <= B; ++i)
        apply(pool, shape, sources, output, i);
}

void BhAdapter::apply(threads::ThreadPool &pool, const cords::AttentionShape &shape, const cords::ViewR &sources, cords::ViewR &output, size_t i)
{

    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);

    std::shared_ptr<ThreadPool> poolPtr(&pool, [](void*){});
    std::vector<BhTraits::Body> bodies(S);
    const float *k_ptr = sources.data() + (i * S * D);
    float *out_ptr     = output.data()  + (i * S * D);

    for (int i = 0; i < S; ++i) {
        auto& p = bodies[i];
        int idx = i * D;
        p.position.x = k_ptr[idx + m_cfg.dim_mapping[0]];
        p.position.y = k_ptr[idx + m_cfg.dim_mapping[1]];
        p.position.z = (D > 2) ?
                           k_ptr[idx + m_cfg.dim_mapping[2]] :
                           0.0f;
        p.mass = 1.0f;
        p.acceleration = {0,0,0};
    }

    auto get_pos = [](const BhTraits::Body &body) -> const BhTraits::Vec3& {
        return body.position;
    };

    auto get_mass = [](const BhTraits::Body &body) -> BhTraits::Real {
        return body.mass;
    };

    // The magic !!
    Solver solver(poolPtr, get_pos, get_mass,
                  m_cfg.theta, m_cfg.gravity, m_cfg.epsilon);

    std::vector<BhTraits::Vec3> forces;
    solver.calculate_forces(bodies, forces);
    for (int i = 0; i < S; ++i) {
        int idx = i * D;
        const auto& f = forces[i];
        out_ptr[idx + m_cfg.dim_mapping[0]] = f.x;
        out_ptr[idx + m_cfg.dim_mapping[1]] = f.y;
        if (D > 2)
            out_ptr[idx + m_cfg.dim_mapping[2]] = f.z;

        if (D > 3)
            std::memset(out_ptr + idx + 3, 0, (D - 3) * sizeof(float));
    }
}

} // namespace job::ai::adapters
