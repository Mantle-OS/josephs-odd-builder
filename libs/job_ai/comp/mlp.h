#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

#include <job_thread_pool.h>
#include <job_parallel_for.h>
#include <simd_provider.h>

#include "gemm.h"


#include "activation.h"
#include "activation_types.h"

namespace job::ai::comp {

using namespace job::simd;

// =========================================================
//  Helpers
// =========================================================

// Element-wise Multiply: A = A * B
inline void hadamardMul(float* __restrict__ A, const float* __restrict__ B, size_t count) {
    size_t i = 0;
    constexpr int K = SIMD::width();

    // Vector Path
    for (; i + K <= count; i += K) {
        auto va = SIMD::pull(A + i);
        auto vb = SIMD::pull(B + i);
        auto res = SIMD::mul(va, vb);
        SIMD::mov(A + i, res);
    }
    // Scalar Tail
    for (; i < count; ++i) {
        A[i] *= B[i];
    }
}

// Parallel Element-wise Multiply
inline void hadamardMulParallel(job::threads::ThreadPool &pool, float* A, const float* B, size_t count) {
    // Determine chunk size (e.g., 4KB blocks to keep L1 cache happy)
    // 1024 floats = 4KB.
    constexpr size_t GRAIN_SIZE = 4096;

    size_t num_chunks = (count + GRAIN_SIZE - 1) / GRAIN_SIZE;

    job::threads::parallel_for(pool, size_t{0}, num_chunks, [&](size_t chunk_idx) {
        size_t start = chunk_idx * GRAIN_SIZE;
        size_t end = std::min(start + GRAIN_SIZE, count);
        size_t len = end - start;

        // Call the serial SIMD helper for this chunk
        hadamardMul(A + start, B + start, len);
    });
}

// =========================================================
//  Standard MLP (Feed Forward)
// =========================================================

inline void mlpForward(int Batch,
                       int d_model,
                       int d_hidden,
                       const float* __restrict__ X,
                       const float* __restrict__ W1,
                       const float* __restrict__ W2,
                       float* __restrict__ HiddenBuf,
                       float* __restrict__ Out,
                       ActivationType act = ActivationType::GELU)
{
    // 1. Up Projection: X [B, d] * W1 [d, h] -> Buf [B, h]
    sgemm(Batch, d_hidden, d_model,
          1.0f, X, d_model, W1,
          d_hidden, 0.0f, HiddenBuf, d_hidden);

    // 2. Activation: Act(Buf)
    // using <false> for T_ESTRIN (Schraudolph) for speed, or <true> for precision
    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;
    activate<false>(HiddenBuf, total_hidden, act);

    // 3. Down Projection: Buf [B, h] * W2 [h, d] -> Out [B, d]
    sgemm(Batch, d_model, d_hidden, 1.0f, HiddenBuf,
          d_hidden, W2, d_model, 0.0f, Out, d_model);
}

inline void mlpParallelForward(job::threads::ThreadPool &pool,
                               int Batch,
                               int d_model,
                               int d_hidden,
                               const float* __restrict__ X,
                               const float* __restrict__ W1,
                               const float* __restrict__ W2,
                               float* __restrict__ HiddenBuf,
                               float* __restrict__ Out,
                               ActivationType act = ActivationType::GELU)
{
    // 1. Parallel Up Projection
    sgemmParallel(pool,
                  Batch, d_hidden, d_model,
                  1.0f, X, d_model, W1,
                  d_hidden, 0.0f, HiddenBuf, d_hidden);

    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;

    // 2. Parallel Activation
    // Note: We need a parallel activation function.
    // Since 'activate' is serial, we wrap it in a parallel_for here manually
    // or add 'activateParallel' to your activations.h later.
    constexpr size_t ACT_GRAIN = 4096;
    size_t num_chunks = (total_hidden + ACT_GRAIN - 1) / ACT_GRAIN;

    job::threads::parallel_for(pool, size_t{0}, num_chunks, [&](size_t i) {
        size_t start = i * ACT_GRAIN;
        size_t end = std::min(start + ACT_GRAIN, total_hidden);
        activate<false>(HiddenBuf + start, end - start, act);
    });

    // 3. Parallel Down Projection
    sgemmParallel(pool, Batch, d_model, d_hidden, 1.0f, HiddenBuf,
                  d_hidden, W2, d_model, 0.0f, Out, d_model);
}

// =========================================================
//  Gated MLP (SwiGLU / GeGLU)
// =========================================================

// Gated MLP: (Activate(X * W_gate) * (X * W_val)) * W_proj
template <bool T_SMOOTH>
inline void gatedMlpForward(int Batch,
                            int d_model,
                            int d_hidden,
                            const float* __restrict__ X,
                            const float* __restrict__ W_gate, // W1
                            const float* __restrict__ W_val,  // V1
                            const float* __restrict__ W_proj, // W2
                            float* __restrict__ GateBuf,      // [B x d_hidden]
                            float* __restrict__ ValBuf,       // [B x d_hidden]
                            float* __restrict__ Out,
                            ActivationType act = ActivationType::SELU)
{
    // 1. Gate Projection: X * W_gate -> GateBuf
    sgemm(Batch, d_hidden, d_model, 1.0f, X,
          d_model, W_gate, d_hidden, 0.0f, GateBuf, d_hidden);

    // 2. Value Projection: X * W_val -> ValBuf
    sgemm(Batch, d_hidden, d_model, 1.0f, X,
          d_model, W_val, d_hidden, 0.0f, ValBuf, d_hidden);

    // 3. Apply Activation to GateBuf (In-Place)
    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;
    // Note: activate() internally handles the switch(act) and SIMD loops
    activate<T_SMOOTH>(GateBuf, total_hidden, act);

    // 4. Element-wise Multiply: GateBuf = GateBuf * ValBuf
    // This replaces the "YIKES" loop with pure SIMD
    hadamardMul(GateBuf, ValBuf, total_hidden);

    // 5. Down Projection: GateBuf * W_proj -> Out
    sgemm(Batch, d_model, d_hidden, 1.0f,
          GateBuf, d_hidden, W_proj, d_model, 0.0f, Out, d_model);
}

inline void gatedParallelMlpForward(job::threads::ThreadPool &pool,
                                    int Batch,
                                    int d_model,
                                    int d_hidden,
                                    const float* __restrict__ X,
                                    const float* __restrict__ W_gate,
                                    const float* __restrict__ W_val,
                                    const float* __restrict__ W_proj,
                                    float* __restrict__ GateBuf,
                                    float* __restrict__ ValBuf,
                                    float* __restrict__ Out,
                                    ActivationType act = ActivationType::SELU)
{
    // 1. Parallel Projections (Ideally, we could run these two concurrently!)
    sgemmParallel(pool, Batch, d_hidden, d_model, 1.0f, X,
                  d_model, W_gate, d_hidden, 0.0f, GateBuf, d_hidden);

    sgemmParallel(pool, Batch, d_hidden, d_model, 1.0f, X,
                  d_model, W_val, d_hidden, 0.0f, ValBuf, d_hidden);

    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;

    // 2. Parallel Activation
    constexpr size_t ACT_GRAIN = 4096;
    size_t num_chunks = (total_hidden + ACT_GRAIN - 1) / ACT_GRAIN;

    job::threads::parallel_for(pool, size_t{0}, num_chunks, [&](size_t i) {
        size_t start = i * ACT_GRAIN;
        size_t end = std::min(start + ACT_GRAIN, total_hidden);

        // A. Activate chunk
        activate<false>(GateBuf + start, end - start, act);

        // B. Multiply chunk
        hadamardMul(GateBuf + start, ValBuf + start, end - start);
    });

    // 3. Parallel Down Projection
    sgemmParallel(pool, Batch, d_model, d_hidden, 1.0f,
                  GateBuf, d_hidden, W_proj, d_model, 0.0f, Out, d_model);
}

// =========================================================
//  Naive Implementations (Benchmarks / Debug)
// =========================================================

inline void mlpNaiveForward(int Batch,
                            int d_model,
                            int d_hidden,
                            const float* __restrict__ X,
                            const float* __restrict__ W1,
                            const float* __restrict__ W2,
                            float* __restrict__ HiddenBuf,
                            float* __restrict__ Out,
                            ActivationType act = ActivationType::GELU)
{
    sgemmNaive(Batch, d_hidden, d_model,
               1.0f, X, d_model, W1,
               d_hidden, 0.0f, HiddenBuf, d_hidden);

    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;
    activateBufferNaive<false>(HiddenBuf, total_hidden, act);

    sgemmNaive(Batch, d_model, d_hidden, 1.0f, HiddenBuf,
               d_hidden, W2, d_model, 0.0f, Out, d_model);
}

inline void gatedNaiveMlpForward(int Batch,
                                 int d_model,
                                 int d_hidden,
                                 const float* __restrict__ X,
                                 const float* __restrict__ W_gate,
                                 const float* __restrict__ W_val,
                                 const float* __restrict__ W_proj,
                                 float* __restrict__ GateBuf,
                                 float* __restrict__ ValBuf,
                                 float* __restrict__ Out,
                                 ActivationType act = ActivationType::SELU)
{
    sgemmNaive(Batch, d_hidden, d_model, 1.0f, X,
               d_model, W_gate, d_hidden, 0.0f, GateBuf, d_hidden);

    sgemmNaive(Batch, d_hidden, d_model, 1.0f, X,
               d_model, W_val, d_hidden, 0.0f, ValBuf, d_hidden);

    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;

    for(size_t i = 0 ; i < total_hidden; i++){ // FIXED: was <=
        float g = activateNaive<false>(GateBuf[i], act);
        GateBuf[i] = g * ValBuf[i];
    }

    sgemmNaive(Batch, d_model, d_hidden, 1.0f,
               GateBuf, d_hidden, W_proj, d_model, 0.0f, Out, d_model);
}

} // namespace job::ai::comp
