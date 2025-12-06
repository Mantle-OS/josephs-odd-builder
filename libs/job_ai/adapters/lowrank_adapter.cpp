#include "lowrank_adapter.h"

#include "gemm.h"
#include "matrix.h"
#include "aligned_allocator.h"

namespace job::ai::adapters {

LowRankAdapter::LowRankAdapter(LowRankConfig cfg) :
    m_cfg(cfg)
{
}

AdapterType LowRankAdapter::type() const
{
    return AdapterType::LowRank;
}

std::string LowRankAdapter::name() const
{
    return "LowRank (Linear Attention)";
}

void LowRankAdapter::adapt(threads::ThreadPool &pool,
                           const cords::AttentionShape &shape,
                           const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                           [[maybe_unused]] const AdapterCtx &ctx)
{
    using namespace job::ai::cords;
    using namespace job::ai::comp;

    const int B = shape.batch;
    const int S = shape.seq;
    const int D = shape.dim;

    job::threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        const float *q_ptr = targets.data() + b * S * D;
        const float *k_ptr = sources.data() + b * S * D;
        const float *v_ptr = values.data()  + b * S * D;
        float *o_ptr       = output.data()  + b * S * D;

        // Scratchpad: K^T [D, S]
        std::vector<float, AlignedAllocator<float, 64>> kT(D * S);
        // Scratchpad: M = K^T * V [D, D] (The "State" Matrix)
        std::vector<float, AlignedAllocator<float, 64>> M_state(D * D);

        // transpose K -> KT
        for(int i=0; i<S; ++i)
            for(int j=0; j<D; ++j)
                kT[j * S + i] = k_ptr[i * D + j];

        Matrix KT(kT.data(), D, S);
        Matrix V(const_cast<float*>(v_ptr), S, D);
        Matrix M(M_state.data(), D, D);

        // [D, S] * [S, D] -> [D, D]
        sgemm(KT, V, M, 1.0f, 0.0f);

        // [S, D] * [D, D] -> [S, D]
        Matrix Q(const_cast<float*>(q_ptr), S, D);
        Matrix O(o_ptr, S, D);

        sgemm(Q, M, O, m_cfg.scale, 0.0f);
    });
}

} // namespace
