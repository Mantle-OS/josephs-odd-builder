#include "dense_adapter.h"

#include <cmath>
#include <algorithm>

#include "gemm.h"
#include "aligned_allocator.h"

#include "matrix.h"

namespace job::ai::adapters {

DenseAdapter::DenseAdapter(DenseConfig cfg):
    m_cfg(cfg)
{
}

AdapterType DenseAdapter::type() const
{
    return AdapterType::Dense;
}

std::string DenseAdapter::name() const
{
    return "Dense (Standard O(N^2))";
}

void DenseAdapter::adaptParallel(threads::ThreadPool &pool, const cords::AttentionShape &shape,
                         const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                         [[maybe_unused]] const AdapterCtx &ctx)
{

    const size_t B = shape.batch;

    job::threads::parallel_for(pool, size_t{0}, B, [&](size_t b) {
        apply(shape, sources, targets, values, output, b);
    });
}

void DenseAdapter::adapt([[maybe_unused]]threads::ThreadPool &pool,
                         const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                         [[maybe_unused]] const AdapterCtx &ctx)
{

    const size_t B = static_cast<size_t>(shape.batch);
    for (size_t i = 0 ; i <= B; ++i)
        apply(shape, sources, targets, values, output, i);
}


void DenseAdapter::apply(const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output, size_t size)
{
    const int S = shape.seq;
    const int D = shape.dim;

    const float *q_ptr = targets.data() + size * S * D;
    const float *k_ptr = sources.data() + size * S * D;
    const float *v_ptr = values.data()  + size * S * D;
    float *o_ptr       = output.data()  + size * S * D;
    std::vector<float, comp::AlignedAllocator<float, 64>> kT(S * D);
    std::vector<float, comp::AlignedAllocator<float, 64>> scores(S * S);

    for(int i = 0; i < S; ++i)
        for(int j=0; j<D; ++j)
            kT[j * S + i] = k_ptr[i * D + j];

    cords::Matrix Q(const_cast<float*>(q_ptr), S, D);
    cords::Matrix KT(kT.data(), D, S);
    cords::Matrix S_Mat(scores.data(), S, S);

    comp::sgemmMatrix(Q, KT, S_Mat, m_cfg.scale, 0.0f);

    for(int i = 0; i < S; ++i) {
        float *row = scores.data() + i * S;
        float maxVal = -1e9f;
        for(int j=0; j<S; ++j)
            maxVal = std::max(maxVal, row[j]);

        float sum = 0.0f;
        for(int j=0; j<S; ++j) {
            row[j] = std::exp(row[j] - maxVal); // FIXME SLOW !!!!
            sum += row[j];
        }
        float invSum = 1.0f / sum;
        for(int j=0; j<S; ++j)
            row[j] *= invSum;
    }

    cords::Matrix V(const_cast<float*>(v_ptr), S, D);
    cords::Matrix O(o_ptr, S, D);

    comp::sgemmMatrix(S_Mat, V, O, 1.0f, 0.0f);
}
}
