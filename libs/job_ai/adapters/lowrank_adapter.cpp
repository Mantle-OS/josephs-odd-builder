#include "lowrank_adapter.h"

#include "gemm.h"
#include "matrix.h"
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

void LowRankAdapter::adaptParallel(threads::ThreadPool &pool,
                           const cords::AttentionShape &shape,
                           const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                           [[maybe_unused]] const AdapterCtx &ctx)
{
    const int B = shape.batch;
    const int S = shape.seq;
    const int D = shape.dim;
    size_t floatsPerBatch = static_cast<size_t>(D) * S + static_cast<size_t>(D) * D;
    cords::AiWeights scratch(floatsPerBatch * B);
    job::threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        apply(S, D, sources, targets, values, output, scratch, floatsPerBatch, b);
    });
}

void LowRankAdapter::adapt([[maybe_unused]]threads::ThreadPool &pool,
                           const cords::AttentionShape &shape,
                           const cords::ViewR &sources,
                           const cords::ViewR &targets,
                           const cords::ViewR &values,
                           cords::ViewR &output,
                           [[maybe_unused]] const AdapterCtx &ctx)
{
    const int B = shape.batch;
    const int S = shape.seq;
    const int D = shape.dim;
    size_t floatsPerBatch = static_cast<size_t>(D) * S + static_cast<size_t>(D) * D;
    comp::AiWeights scratch(floatsPerBatch * B);
    for(size_t i = 0; i <= size_t(B); i++)
        apply(S, D, sources, targets, values, output, scratch, floatsPerBatch, i);
}


void LowRankAdapter::apply(int S, int D,
                           const cords::ViewR &sources,
                           const cords::ViewR &targets,
                           const cords::ViewR &values,
                           cords::ViewR &output,
                           cords::AiWeights scratch,
                           size_t floatsPerBatch,
                           size_t size
                           )
{
    const float *q_ptr = targets.data() + size * S * D;
    const float *k_ptr = sources.data() + size * S * D;
    const float *v_ptr = values.data()  + size * S * D;
    float *o_ptr       = output.data()  + size * S * D;

    float *batch_scratch = scratch.data() + (size * floatsPerBatch);
    float *kt_ptr = batch_scratch;
    float *m_ptr  = batch_scratch + (D * S);

    comp::transpose(k_ptr, kt_ptr, S, D);

    cords::Matrix KT(kt_ptr, D, S);
    cords::Matrix V(const_cast<float*>(v_ptr), S, D);
    cords::Matrix M(m_ptr, D, D);

    // [D, S] * [S, D] -> [D, D]
    comp::sgemmMatrix(KT, V, M, 1.0f, 0.0f);

    // [S, D] * [D, D] -> [S, D]
    cords::Matrix Q(const_cast<float*>(q_ptr), S, D);
    cords::Matrix O(o_ptr, S, D);

    comp::sgemmMatrix(Q, M, O, m_cfg.scale, 0.0f);
}


}
