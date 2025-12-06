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

void DenseAdapter::adapt(threads::ThreadPool &pool, const cords::AttentionShape &shape,
                         const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output,
                         [[maybe_unused]] const AdapterCtx &ctx)
{
    using namespace job::ai::cords;
    using namespace job::ai::comp;

    const int B = shape.batch;
    const int S = shape.seq;
    const int D = shape.dim;
    job::threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t b) {
        const float* q_ptr = targets.data() + b * S * D;
        const float* k_ptr = sources.data() + b * S * D;
        const float* v_ptr = values.data()  + b * S * D;
        float* o_ptr       = output.data()  + b * S * D;
        std::vector<float, AlignedAllocator<float, 64>> kT(S * D);
        std::vector<float, AlignedAllocator<float, 64>> scores(S * S);

        for(int i=0; i<S; ++i)
            for(int j=0; j<D; ++j)
                kT[j * S + i] = k_ptr[i * D + j];

        Matrix Q(const_cast<float*>(q_ptr), S, D);
        Matrix KT(kT.data(), D, S);
        Matrix S_Mat(scores.data(), S, S);

        sgemm(Q, KT, S_Mat, m_cfg.scale, 0.0f);

        for(int i=0; i<S; ++i) {
            float* row = scores.data() + i * S;
            float maxVal = -1e9f;
            for(int j=0; j<S; ++j)
                maxVal = std::max(maxVal, row[j]);

            float sum = 0.0f;
            for(int j=0; j<S; ++j) {
                row[j] = std::exp(row[j] - maxVal);
                sum += row[j];
            }
            float invSum = 1.0f / sum;
            for(int j=0; j<S; ++j) row[j] *= invSum;
        }

        Matrix V(const_cast<float*>(v_ptr), S, D);
        Matrix O(o_ptr, S, D);

        sgemm(S_Mat, V, O, 1.0f, 0.0f);
    });
}


} // namespace
