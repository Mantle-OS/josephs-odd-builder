#include "lowrank_adapter.h"

#include "gemm.h"
#include "matrix.h"
#include "aligned_allocator.h"
#include "transpose.h"

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
    const int B = shape.batch;
    const int S = shape.seq;
    const int D = shape.dim;

    size_t floatsPerBatch = static_cast<size_t>(D) * S + static_cast<size_t>(D) * D;
    size_t totalFloats = floatsPerBatch * B;

    std::vector<float, comp::AlignedAllocator<float, 64>> scratch(totalFloats);

    job::threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        const float *q_ptr = targets.data() + b * S * D;
        const float *k_ptr = sources.data() + b * S * D;
        const float *v_ptr = values.data()  + b * S * D;
        float *o_ptr       = output.data()  + b * S * D;

        float *batch_scratch = scratch.data() + (b * floatsPerBatch);
        float *kt_ptr = batch_scratch;
        float *m_ptr  = batch_scratch + (D * S);

        comp::transpose(k_ptr, kt_ptr, S, D);

        cords::Matrix KT(kt_ptr, D, S);
        cords::Matrix V(const_cast<float*>(v_ptr), S, D);
        cords::Matrix M(m_ptr, D, D);

        // [D, S] * [S, D] -> [D, D]
        comp::sgemm(KT, V, M, 1.0f, 0.0f);

        // [S, D] * [D, D] -> [S, D]
        cords::Matrix Q(const_cast<float*>(q_ptr), S, D);
        cords::Matrix O(o_ptr, S, D);

        comp::sgemm(Q, M, O, m_cfg.scale, 0.0f);
    });
}

}
