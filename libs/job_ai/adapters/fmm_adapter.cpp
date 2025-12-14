#include "fmm_adapter.h"

#include <cstring>
#include <cmath>
#include <vector>

#include <job_fmm_integrator.h>

namespace job::ai::adapters {

using namespace job::ai::cords;
using namespace job::threads;

FmmAdapter::FmmAdapter(FmmConfig cfg) :
    m_cfg(cfg)
{
}

AdapterType FmmAdapter::type() const
{
    return AdapterType::FMM;
}

std::string FmmAdapter::name() const
{
    return "FMM (Fast Multipole Method)";
}

void FmmAdapter::adapt(
    job::threads::ThreadPool &pool,
    const AttentionShape &shape,
    const ViewR &sources,                       // Keys (The Mass Sources)
    [[maybe_unused]] const ViewR &targets,      // Queries (The Test Particles)
    [[maybe_unused]] const ViewR &values,       // Values (The Payload/Charge)
    ViewR &output,                              // The Resulting Field (Context)
    [[maybe_unused]] const AdapterCtx &ctx
    )
{
    // dimensions
    const int B = static_cast<int>(shape.batch);
    const int S = static_cast<int>(shape.seq);
    const int D = static_cast<int>(shape.dim);

    job::threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        std::vector<FmmTraits::Body> bodies(S);

        // Pointers to this batch's data slice
        // !!! NOTE !!!!!! this assume RowMajor layout (stride = D) :(
        const float *k_ptr = sources.data() + (b * S * D);
        // const float *q_ptr = targets.data() + (b * S * D);

        // IDK if I should Use V as the "Charge" magnitude ..."I think"  stick to: K determines Pos & Mass ?
        // const float *v_ptr = values.data() + (b * S * D);

        float *out_ptr = output.data() + (b * S * D);

        for (int i = 0; i < S; ++i) {
            auto &p = bodies[i];
            int idx = i * D;

            p.position.x = k_ptr[idx + m_cfg.dim_mapping[0]];
            p.position.y = k_ptr[idx + m_cfg.dim_mapping[1]];

            // Handle 2D embeddings gracefully (e.g. RoPE)
            if (D > 2)
                p.position.z = k_ptr[idx + m_cfg.dim_mapping[2]];
            else
                p.position.z = 0.0f;

            p.mass = 1.0f;
            p.acceleration = {0,0,0};
            p.id = i;
        }


        // The Math Dragon: FMM Execution
        typename FmmSolver::Params fmmParams;
        fmmParams.theta       = m_cfg.theta;
        fmmParams.maxLeafSize = m_cfg.maxLeaf;
        fmmParams.maxDepth    = m_cfg.maxDepth;
        FmmSolver engine(pool, fmmParams);

        // P2M -> M2M -> M2L -> L2L -> L2P  LETS FUCKING GO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        engine.compute(bodies);

        // decoder: Force -> Output Tensor
        // The "Force" felt by a token is the aggregate "Attention" it received. map this force vector back into the embedding.
        for (const auto &p : bodies) {
            // Use p.id because FMM might have reordered the vector ?
            int i = p.id;
            int idx = i * D;

            // write context vector (force) apply gravity constant (G)
            float fx = p.acceleration.x * m_cfg.gravity;
            float fy = p.acceleration.y * m_cfg.gravity;
            float fz = p.acceleration.z * m_cfg.gravity;

            out_ptr[idx + m_cfg.dim_mapping[0]] = fx;
            out_ptr[idx + m_cfg.dim_mapping[1]] = fy;
            if (D > 2)
                out_ptr[idx + m_cfg.dim_mapping[2]] = fz;

            // 0 out the non-spatial dimensions for now. In a residual stream, might just add this to the input.
            // For now, write zeros to be clean (Sparse output).
            if (D > 3)
                std::memset(out_ptr + idx + 3, 0, (D - 3) * sizeof(float));
        }
    });
}

} // namespace job::ai::adapters
