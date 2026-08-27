#include "dense_adapter.h"

#include <cmath>
#include <algorithm>

#include <aligned_allocator.h>

#include <simd_for.h>

#include "gemm.h"

#include "matrix.h"

namespace job::ai::adapters {
using namespace job::simd;
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
    for (size_t i = 0 ; i < B; ++i)
        apply(shape, sources, targets, values, output, i);
}

void DenseAdapter::apply(const cords::AttentionShape &shape, const cords::ViewR &sources, const cords::ViewR &targets, const cords::ViewR &values, cords::ViewR &output, size_t batchIdx)
{
    const int S = shape.seq;
    const int D = shape.dim;

    const float *q_ptr = targets.data() + batchIdx * S * D;
    const float *k_ptr = sources.data() + batchIdx * S * D;
    const float *v_ptr = values.data()  + batchIdx * S * D;
    float *o_ptr       = output.data()  + batchIdx * S * D;

    // We transpose K to KT for standard GEMM (Q * KT)
    std::vector<float, core::AlignedAllocator<float, 64>> kT(S * D);
    std::vector<float, core::AlignedAllocator<float, 64>> scores(S * S);

    // Transpose K -> KT write a real transpose later [[COME_BACK]]
    // (This is memory bound, scalar is usually fine for "Dense" baseline)
    for(int i = 0; i < S; ++i)
        for(int j=0; j<D; ++j)
            kT[j * S + i] = k_ptr[i * D + j];

    cords::Matrix Q(const_cast<float*>(q_ptr), S, D);
    cords::Matrix KT(kT.data(), D, S);
    cords::Matrix S_Mat(scores.data(), S, S);

    // Compute Scores: S = Q * K^T
    comp::sgemmMatrix(Q, KT, S_Mat, m_cfg.scale, 0.0f);

    // Softmax
    for(int i = 0; i < S; ++i) {
        float *row = scores.data() + i * S;

        // Find Max (Scalar is fine for reduction usually, or hmax helper)
        float maxVal = -1e9f;
        for(int j=0; j<S; ++j) {
            if (row[j] > maxVal) maxVal = row[j];
        }

        // Exponentials and Sum
        auto v_max = SIMD::set1(maxVal);
        f32 v_sum = SIMD::zero();

        threads::simd_for(S,
                 [&](size_t j) {
                     auto v_val = SIMD::pull(row + j);
                     auto v_exp = SIMD::exp_fast(SIMD::sub(v_val, v_max)); // Fast Schraudolph
                     SIMD::mov(row + j, v_exp);
                     v_sum = SIMD::add(v_sum, v_exp);
                 },
                 [&](size_t j) {
                     // Tail fallback using scalar fast exp logic
                     // Or just std::exp since tails are small
                     row[j] = std::exp(row[j] - maxVal);
                 }
                 );

        // Horizontal Sum
        float sum = job::simd::hsum(v_sum);
        // Add tail elements to sum (since v_sum only tracked vector parts)
        // Wait, simd_for doesn't reduce tails into v_sum ?  [[COME_BACK]]
        // We need to accumulate tails manually or redesign the lambda capture.
        size_t tail_start = (S / SIMD::width()) * SIMD::width();
        for(size_t j = tail_start; j < size_t(S); ++j)
            sum += row[j];

        // C. Normalize
        float invSum = 1.0f / sum;
        auto v_inv = SIMD::set1(invSum);

        threads::simd_for(S,
                 [&](size_t j) {
                     SIMD::mov(row + j, SIMD::mul(SIMD::pull(row + j), v_inv));
                 },
                 [&](size_t j) {
                     row[j] *= invSum;
                 }
                 );
    }

    // O = S * V
    cords::Matrix V(const_cast<float*>(v_ptr), S, D);
    cords::Matrix O(o_ptr, S, D);

    comp::sgemmMatrix(S_Mat, V, O, 1.0f, 0.0f);
}
} // namespace job::ai::adapter
