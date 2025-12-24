#pragma once
#include <algorithm>

#include <job_parallel_for.h>

#include <simd_provider.h>

#include "activation_types.h"

#include "elu.h"
#include "gelu.h"
#include "identity.h"
#include "mish.h"
#include "relu.h"
#include "sigmoid.h"
#include "softplus.h"
// #include "special.h" // Later these will be Maxout and GDN
#include "swish.h"
#include "tanh.h"
#include "swish.h"

namespace job::ai::comp {
using namespace job::simd;


// Helper: Masked Load (Prevents reading garbage/segfaults at edge of W)
static inline f32 maskedLoad(const float* ptr, int count) {
    if (count >= SIMD::width()) {
        return SIMD::pull(ptr);
    } else {
        alignas(sizeof(float) * SIMD::width()) float tmp[SIMD::width()] = {};
        for (int i = 0; i < count; ++i)
            tmp[i] = ptr[i];
        return SIMD::pull(tmp);
    }
}

// Helper: Masked Store (Reuse logic from gemm.h)
static inline void maskedStore(float* ptr, f32 val, int count) {
    if (count >= SIMD::width()) {
        SIMD::mov(ptr, val);
    } else {
        alignas(sizeof(float) * SIMD::width()) float tmp[SIMD::width()] = {};
        SIMD::mov(tmp, val);
        for (int i = 0; i < count; ++i)
            ptr[i] = tmp[i];
    }
}

template <typename OP, bool UseBias>
inline void microKernel1xXMasked(
    int K,
    const float* __restrict__ A,                // Single row of A
    const float* __restrict__ W, int ldw,       // W ptr
    const float* __restrict__ B,                // Bias ptr
    float* __restrict__ O,                      // Single row output
    float actAlpha,
    int maskWidth = SIMD::width()               // If < 8, use masked load/store
    ) {
    f32 acc;

    // accumulator
    if (UseBias)
        acc = maskedLoad(B, maskWidth);
    else
        acc = SIMD::zero();

    // Compute
    for (int k = 0; k < K; ++k) {
        auto in_val = SIMD::set1(A[k]);
        auto w_vec = maskedLoad(W + k * ldw, maskWidth);
        acc = SIMD::mul_plus(in_val, w_vec, acc);
    }

    // Activation
    acc = OP::apply(acc, actAlpha);

    // store
    maskedStore(O, acc, maskWidth);
}
#if defined(__AVX512F__)
template <typename OP, bool UseBias>
__attribute__((always_inline))
inline void microKernel16xX(int K,
               const float* __restrict__ A, int lda,       // A points to A[row, 0]
               const float* __restrict__ W, int ldw,       // W points to W[0, col]
               const float* __restrict__ B,                // B points to B[col]
               float* __restrict__ O, int ldo,             // O points to O[row, col]
               float actAlpha)
{

    f32 b_vec = UseBias ? SIMD::pull(B) : SIMD::zero();

    f32 acc0  = b_vec; // Row 0
    f32 acc1  = b_vec; // Row 1
    f32 acc2  = b_vec; // ...
    f32 acc3  = b_vec;
    f32 acc4  = b_vec;
    f32 acc5  = b_vec;
    f32 acc6  = b_vec;
    f32 acc7  = b_vec;
    f32 acc8  = b_vec;
    f32 acc9  = b_vec;
    f32 acc10 = b_vec;
    f32 acc11 = b_vec;
    f32 acc12 = b_vec;
    f32 acc13 = b_vec;
    f32 acc14 = b_vec;
    f32 acc15 = b_vec;

    // unroll slightly to hide latency if needed, but 8x16 is register heavy already.
    for (int k = 0; k < K; ++k) {
        f32 w_vec = SIMD::pull(W + k * ldw);
        acc0  = SIMD::mul_plus(SIMD::set1(A[0  * lda + k]), w_vec, acc0);
        acc1  = SIMD::mul_plus(SIMD::set1(A[1  * lda + k]), w_vec, acc1);
        acc2  = SIMD::mul_plus(SIMD::set1(A[2  * lda + k]), w_vec, acc2);
        acc3  = SIMD::mul_plus(SIMD::set1(A[3  * lda + k]), w_vec, acc3);
        acc4  = SIMD::mul_plus(SIMD::set1(A[4  * lda + k]), w_vec, acc4);
        acc5  = SIMD::mul_plus(SIMD::set1(A[5  * lda + k]), w_vec, acc5);
        acc6  = SIMD::mul_plus(SIMD::set1(A[6  * lda + k]), w_vec, acc6);
        acc7  = SIMD::mul_plus(SIMD::set1(A[7  * lda + k]), w_vec, acc7);
        acc8  = SIMD::mul_plus(SIMD::set1(A[8  * lda + k]), w_vec, acc8);
        acc9  = SIMD::mul_plus(SIMD::set1(A[9  * lda + k]), w_vec, acc9);
        acc10 = SIMD::mul_plus(SIMD::set1(A[10 * lda + k]), w_vec, acc10);
        acc11 = SIMD::mul_plus(SIMD::set1(A[11 * lda + k]), w_vec, acc11);
        acc12 = SIMD::mul_plus(SIMD::set1(A[12 * lda + k]), w_vec, acc12);
        acc13 = SIMD::mul_plus(SIMD::set1(A[13 * lda + k]), w_vec, acc13);
        acc14 = SIMD::mul_plus(SIMD::set1(A[14 * lda + k]), w_vec, acc14);
        acc15 = SIMD::mul_plus(SIMD::set1(A[15 * lda + k]), w_vec, acc15);
    }

    // Activation & Store
    SIMD::mov(O + 0  * ldo, OP::apply(acc0, actAlpha));
    SIMD::mov(O + 1  * ldo, OP::apply(acc1, actAlpha));
    SIMD::mov(O + 2  * ldo, OP::apply(acc2, actAlpha));
    SIMD::mov(O + 3  * ldo, OP::apply(acc3, actAlpha));
    SIMD::mov(O + 4  * ldo, OP::apply(acc4, actAlpha));
    SIMD::mov(O + 5  * ldo, OP::apply(acc5, actAlpha));
    SIMD::mov(O + 6  * ldo, OP::apply(acc6, actAlpha));
    SIMD::mov(O + 7  * ldo, OP::apply(acc7, actAlpha));
    SIMD::mov(O + 8  * ldo, OP::apply(acc8, actAlpha));
    SIMD::mov(O + 9  * ldo, OP::apply(acc9, actAlpha));
    SIMD::mov(O + 10 * ldo, OP::apply(acc10, actAlpha));
    SIMD::mov(O + 11 * ldo, OP::apply(acc11, actAlpha));
    SIMD::mov(O + 12 * ldo, OP::apply(acc12, actAlpha));
    SIMD::mov(O + 13 * ldo, OP::apply(acc13, actAlpha));
    SIMD::mov(O + 14 * ldo, OP::apply(acc14, actAlpha));
    SIMD::mov(O + 15 * ldo, OP::apply(acc15, actAlpha));
}
#elif defined(__AVX2__) || defined(__ARM_NEON) || defined(__aarch64__) || defined(__AVX512F__)
template <typename OP, bool UseBias>
__attribute__((always_inline))
inline void microKernel8xX(int K,
               const float* __restrict__ A, int lda,       // A points to A[row, 0]
               const float* __restrict__ W, int ldw,       // W points to W[0, col]
               const float* __restrict__ B,                // B points to B[col]
               float* __restrict__ O, int ldo,             // O points to O[row, col]
               float actAlpha)
{
    // Initialize Accumulators
    // If Bias is used, we load it once and set accs to it.  Otherwise zero.
    f32 b_vec = UseBias ? SIMD::pull(B) : SIMD::zero();

    f32 acc0 = b_vec;
    f32 acc1 = b_vec;
    f32 acc2 = b_vec;
    f32 acc3 = b_vec;
    f32 acc4 = b_vec;
    f32 acc5 = b_vec;
    f32 acc6 = b_vec;
    f32 acc7 = b_vec;

    // unroll slightly to hide latency if needed, but 8x8 is register heavy already.
    for (int k = 0; k < K; ++k) {
        // Load 1 row of weights (8 columns) -> Reused 8 times!
        f32 w_vec = SIMD::pull(W + k * ldw);

        // Broadcast A scalars for the 8 rows
        // FMA: acc += A[r, k] * W[k, c..c+7]
        acc0 = SIMD::mul_plus(SIMD::set1(A[0 * lda + k]), w_vec, acc0);
        acc1 = SIMD::mul_plus(SIMD::set1(A[1 * lda + k]), w_vec, acc1);
        acc2 = SIMD::mul_plus(SIMD::set1(A[2 * lda + k]), w_vec, acc2);
        acc3 = SIMD::mul_plus(SIMD::set1(A[3 * lda + k]), w_vec, acc3);
        acc4 = SIMD::mul_plus(SIMD::set1(A[4 * lda + k]), w_vec, acc4);
        acc5 = SIMD::mul_plus(SIMD::set1(A[5 * lda + k]), w_vec, acc5);
        acc6 = SIMD::mul_plus(SIMD::set1(A[6 * lda + k]), w_vec, acc6);
        acc7 = SIMD::mul_plus(SIMD::set1(A[7 * lda + k]), w_vec, acc7);
    }

    // Activation & Store
    SIMD::mov(O + 0 * ldo, OP::apply(acc0, actAlpha));
    SIMD::mov(O + 1 * ldo, OP::apply(acc1, actAlpha));
    SIMD::mov(O + 2 * ldo, OP::apply(acc2, actAlpha));
    SIMD::mov(O + 3 * ldo, OP::apply(acc3, actAlpha));
    SIMD::mov(O + 4 * ldo, OP::apply(acc4, actAlpha));
    SIMD::mov(O + 5 * ldo, OP::apply(acc5, actAlpha));
    SIMD::mov(O + 6 * ldo, OP::apply(acc6, actAlpha));
    SIMD::mov(O + 7 * ldo, OP::apply(acc7, actAlpha));
}
#endif


template <typename OP, bool UseBias>
void fusedDenseKernel(
    const float* __restrict__ A,
    const float* __restrict__ W,
    const float* __restrict__ B,
    float* __restrict__ O,
    int rows,
    int inFeatures,
    int outFeatures,
    float alpha)
{
    constexpr int VECLEN = SIMD::width();   // 8 on AVX2, 16 on AVX512
    constexpr int MR = 8;                   // Row Blocking Factor  8
    constexpr int NR = VECLEN;              // Col Blocking Factor (SIMD Width)

    int rb = 0;

    // process 8 rows at a time
    for (; rb <= rows - MR; rb += MR) {
        int cb = 0;

        // 8xX Blockswhere X is SIMD::width
        for (; cb <= outFeatures - NR; cb += NR) {
            microKernel8xX<OP, UseBias>(
                inFeatures,                                 // make sure this is 8 ?
                A + rb * inFeatures, inFeatures,            // A ptr, stride
                W + cb, outFeatures,                        // W ptr, stride
                B + cb,                                     // Bias ptr
                O + rb * outFeatures + cb, outFeatures,     // Out ptr, stride
                alpha
                );
        }

        // Right Edge Remaining Columns 1xX where X is SIMD::width
        if (cb < outFeatures) {
            int rem = outFeatures - cb;
            // Fallback: Loop over the 8 rows, handle the tail columns for each
            for (int r = 0; r < MR; ++r) {
                microKernel1xXMasked<OP, UseBias>(
                    inFeatures,
                    A + (rb + r) * inFeatures,
                    W + cb, outFeatures,
                    B + cb,
                    O + (rb + r) * outFeatures + cb,
                    alpha,
                    rem // Pass mask width
                    );
            }
        }
    }

    // Bottom Edge: Process remaining rows (1..7) each remaining row as a 1xN GEMV
    for (; rb < rows; ++rb) {
        int cb = 0;
        for (; cb <= outFeatures - NR; cb += NR) {
            microKernel1xXMasked<OP, UseBias>(
                inFeatures,
                A + rb * inFeatures,
                W + cb, outFeatures,
                B + cb,
                O + rb * outFeatures + cb,
                alpha,
                SIMD::width() // Full width
                );
        }
        // right edge of bottom rows
        if (cb < outFeatures) {
            microKernel1xXMasked<OP, UseBias>(
                inFeatures,
                A + rb * inFeatures,
                W + cb, outFeatures,
                B + cb,
                O + rb * outFeatures + cb,
                alpha,
                outFeatures - cb
                );
        }
    }
}

template<bool T_ESTRIN>
inline void activateDense(
    const float* __restrict__ A,
    const float* __restrict__ W,
    const float* __restrict__ B,
    float* __restrict__ O,
    int rows, int in, int out,
    ActivationType type,
    bool useBias,
    float alpha = 1.0f)
{
    // Helper to instantiate the correct kernel template
    auto dispatch = [&](auto tag_bias) {
        constexpr bool Bias = decltype(tag_bias)::value;
        switch (type) {
        case ActivationType::Identity:
            fusedDenseKernel<FunctorIdentity<T_ESTRIN>, Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::Sigmoid:
            fusedDenseKernel<FunctorSigmoid<T_ESTRIN>,  Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::Tanh:
            fusedDenseKernel<FunctorTann<T_ESTRIN>,     Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::HardTanh:
            fusedDenseKernel<FunctorHardTanh<T_ESTRIN>, Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::ReLU:
            fusedDenseKernel<FunctorReLU<T_ESTRIN>,     Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::LeakyReLU:
            fusedDenseKernel<FunctorLeakyReLU<T_ESTRIN>,Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::PReLU:
            fusedDenseKernel<FunctorPReLU<T_ESTRIN>,    Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::RReLU:
            fusedDenseKernel<FunctorRReLU<T_ESTRIN>,    Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::ELU:
            fusedDenseKernel<FunctorELU<T_ESTRIN>,      Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::SELU:
            fusedDenseKernel<FunctorSELU<T_ESTRIN>,     Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::GELU:
            fusedDenseKernel<FunctorGELU<T_ESTRIN>,     Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::AproxGELU:
            fusedDenseKernel<FunctorAproxGELU<T_ESTRIN>,Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::Swish:
            fusedDenseKernel<FunctorSwish<T_ESTRIN>,    Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::HSwish:
            fusedDenseKernel<FunctorHardSwish<T_ESTRIN>,Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::Mish:
            fusedDenseKernel<FunctorMish<T_ESTRIN>,     Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::HMish:
            fusedDenseKernel<FunctorHardMish<T_ESTRIN>, Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        case ActivationType::Softplus:
            fusedDenseKernel<FunctorSoftplus<T_ESTRIN>, Bias>(A, W, B, O, rows, in, out, alpha);
            break;
        default: break; // Fallback or Error
        }
    };

    if (useBias)
        dispatch(std::true_type{});
    else
        dispatch(std::false_type{});
}

// Parallel Wrapper: Chunks the rows and dispatches
template<bool T_ESTRIN>
inline void activateDenseParallel(
    job::threads::ThreadPool &pool,
    const float* __restrict__ A,
    const float* __restrict__ W,
    const float* __restrict__ B,
    float* __restrict__ O,
    int rows, int in, int out,
    ActivationType type,
    bool useBias,
    float alpha = 1.0f)
{
    // Heuristic: Ensure each thread gets enough work (e.g. 16 rows)
    constexpr int kMinRowsPerThread = 16;
    int threadCount = static_cast<int>(pool.workerCount()) + 1; // +1 for main thread if stealing
    int chunk = std::max(kMinRowsPerThread, (rows + threadCount - 1) / threadCount);

    int numChunks = (rows + chunk - 1) / chunk;

    job::threads::parallel_for(pool, size_t{0}, size_t(numChunks), [&](size_t i) {
        int r_start = static_cast<int>(i) * chunk;
        int r_end   = std::min(r_start + chunk, rows);
        int r_count = r_end - r_start;

        if (r_count > 0) {
            // Pointer arithmetic for sub-slice
            const float* A_sub = A + (r_start * in);
            float* O_sub = O + (r_start * out);

            activateDense<T_ESTRIN>(A_sub, W, B, O_sub, r_count, in, out, type, useBias, alpha);
        }
    });
}


template<bool T_ESTRIN>
inline void activateDense(float* __restrict__ data,
                          size_t size,
                          ActivationType type,
                          float alpha = 1.0f) // We can get rid of smooth then go through these calls fixing the
{
    switch (type) {
    case ActivationType::Identity:
        fusedDenseKernel<FunctorIdentity<T_ESTRIN>, false>(data, size, alpha);
        break;
    case ActivationType::Sigmoid:
        fusedDenseKernel<FunctorSigmoid<T_ESTRIN>, false>(data, size, alpha);
        break;
    case ActivationType::Tanh:
        fusedDenseKernel<FunctorTann<T_ESTRIN>, false>(data, size, alpha);
        break;
    case ActivationType::HardTanh:
        fusedDenseKernel<FunctorHardTanh<T_ESTRIN>, false>(data, size, alpha);
        break;
    case ActivationType::ReLU:
        fusedDenseKernel<FunctorReLU<T_ESTRIN>, false>(data, size, alpha);
        break;
    case ActivationType::LeakyReLU:
        fusedDenseKernel<FunctorLeakyReLU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::PReLU:
        fusedDenseKernel<FunctorPReLU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::RReLU:
        fusedDenseKernel<FunctorRReLU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::ELU:
        fusedDenseKernel<FunctorELU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::SELU:
        fusedDenseKernel<FunctorSELU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::GELU:
        fusedDenseKernel<FunctorGELU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::AproxGELU:
        fusedDenseKernel<FunctorAproxGELU<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::Swish:
        fusedDenseKernel<FunctorSwish<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::HSwish:
        fusedDenseKernel<FunctorHardSwish<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::Mish:
        fusedDenseKernel<FunctorMish<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::HMish:
        fusedDenseKernel<FunctorHardMish<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::Softplus:
        fusedDenseKernel<FunctorSoftplus<T_ESTRIN>, true>(data, size, alpha);
        break;
    case ActivationType::Maxout:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::maxout(range) on its own");
        break;
    case ActivationType::GDN:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::rrelu(x, config)"); // I guess we could call this as well now
        break;
    }
}


template<typename T_FUNCTOR>
inline void activationKernelImpl(float* __restrict__ data,
                                 size_t n,
                                 float alpha)
{
    size_t i = 0;
    constexpr int K = SIMD::width();
    for (; i + (K-1) < n; i += K) {
        auto v = SIMD::pull(data + i);
        v = T_FUNCTOR::apply(v, alpha);
        SIMD::mov(data + i, v);
    }
}

template<bool T_ESTRIN>
inline void activate(float* __restrict__ data,
                     size_t size,
                     ActivationType type,
                     float alpha = 1.0f)
{
    switch (type) {
    case ActivationType::Identity:
        activationKernelImpl<FunctorIdentity<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::Sigmoid:
        activationKernelImpl<FunctorSigmoid<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::Tanh:
        activationKernelImpl<FunctorTann<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::HardTanh:
        activationKernelImpl<FunctorHardTanh<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::ReLU:
        activationKernelImpl<FunctorReLU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::LeakyReLU:
        activationKernelImpl<FunctorLeakyReLU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::PReLU:
        activationKernelImpl<FunctorPReLU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::RReLU:
        activationKernelImpl<FunctorRReLU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::ELU:
        activationKernelImpl<FunctorELU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::SELU:
        activationKernelImpl<FunctorSELU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::GELU:
        activationKernelImpl<FunctorGELU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::AproxGELU:
        activationKernelImpl<FunctorAproxGELU<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::Swish:
        activationKernelImpl<FunctorSwish<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::HSwish:
        activationKernelImpl<FunctorHardSwish<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::Mish:
        activationKernelImpl<FunctorMish<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::HMish:
        activationKernelImpl<FunctorHardMish<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::Softplus:
        activationKernelImpl<FunctorSoftplus<T_ESTRIN>>(data, size, alpha);
        break;
    case ActivationType::Maxout:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::maxout(range) on its own");
        break;
    case ActivationType::GDN:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::rrelu(x, config)"); // I guess we could call this as well now
        break;
    }
}


template<bool T_ESTRIN>
inline f32 activateSingle(float* __restrict__ data,
                          ActivationType type,
                          float alpha = 1.0f)
{
    switch (type) {
    case ActivationType::Identity:
        return FunctorIdentity<T_ESTRIN>::apply(data, alpha);
    case ActivationType::Sigmoid:
        return FunctorSigmoid<T_ESTRIN>::apply(data, alpha);
    case ActivationType::Tanh:
        return FunctorTann<T_ESTRIN>::apply(data, alpha);
    case ActivationType::HardTanh:
        return FunctorHardTanh<T_ESTRIN>::apply(data, alpha);
    case ActivationType::ReLU:
        return FunctorReLU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::LeakyReLU:
        return FunctorLeakyReLU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::PReLU:
        return FunctorPReLU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::RReLU:
        return FunctorRReLU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::ELU:
        return FunctorELU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::SELU:
        return FunctorSELU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::GELU:
        return FunctorGELU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::AproxGELU:
        return FunctorAproxGELU<T_ESTRIN>::apply(data, alpha);
    case ActivationType::Swish:
        return FunctorSwish<T_ESTRIN>::apply(data, alpha);
    case ActivationType::HSwish:
        return FunctorHardSwish<T_ESTRIN>::apply(data, alpha);
    case ActivationType::Mish:
        return FunctorMish<T_ESTRIN>::apply(data, alpha);
    case ActivationType::HMish:
        return FunctorHardMish<T_ESTRIN>::apply(data, alpha);
    case ActivationType::Softplus:
        return FunctorSoftplus<T_ESTRIN>::apply(data, alpha);
    case ActivationType::Maxout:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::maxout(range) on its own");
        break;
    case ActivationType::GDN:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::rrelu(x, config)"); // I guess we could call this as well now
        break;
    }
    return SIMD::zero();
}

template<bool T_ESTRIN>
inline float activateNaive(float x, ActivationType type, float alpha = 0.0f)
{
    const float param = alpha;
    switch (type) {
    case ActivationType::Identity:
        return FunctorIdentity<T_ESTRIN>::apply_naive(x);
    case ActivationType::Sigmoid:
        return FunctorSigmoid<T_ESTRIN>::apply_naive(x);
    case ActivationType::Tanh:
        return FunctorTann<T_ESTRIN>::apply_naive(x);
    case ActivationType::HardTanh:
        return FunctorHardTanh<T_ESTRIN>::apply_naive(x);
    case ActivationType::ReLU:
        return FunctorReLU<T_ESTRIN>::apply_naive(x);
    case ActivationType::LeakyReLU: {
        const float a = (param != 0.0f) ? param : 0.01f;
        return FunctorLeakyReLU<T_ESTRIN>::apply_naive(x, a);
    }
    case ActivationType::PReLU:
        return FunctorPReLU<T_ESTRIN>::apply_naive(x, param);
    case ActivationType::RReLU:
        return FunctorRReLU<T_ESTRIN>::apply_naive(x, param);
    case ActivationType::ELU:
        return FunctorELU<T_ESTRIN>::apply_naive(x, param);
    case ActivationType::SELU:
        return FunctorSELU<T_ESTRIN>::apply_naive(x);
    case ActivationType::GELU:
        return FunctorGELU<T_ESTRIN>::apply_naive(x);
    case ActivationType::AproxGELU:
        return FunctorAproxGELU<T_ESTRIN>::apply_naive(x);
    case ActivationType::Swish:{
        const float beta = (param != 0.0f) ? param : 1.0f;
        return FunctorSwish<T_ESTRIN>::apply_naive(x, beta);
    }
    case ActivationType::HSwish:
        return FunctorHardSwish<T_ESTRIN>::apply_naive(x);
    case ActivationType::Mish:
        return FunctorMish<T_ESTRIN>::apply_naive(x);
    case ActivationType::HMish:
        return FunctorHardMish<T_ESTRIN>::apply_naive(x);
    case ActivationType::Softplus:
        return FunctorSoftplus<T_ESTRIN>::apply_naive(x);
    case ActivationType::Maxout:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::maxout(range) on its own");
        break;
    case ActivationType::GDN:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::rrelu(x, config)");
        break;
    }
    return 0.0f;
}


template<bool T_ESTRIN>
inline void activateBufferNaive(float *buffer, size_t count, ActivationType type, float alpha = 0.0f)
{
    for (size_t i = 0; i < count; ++i)
        buffer[i] = activateNaive<T_ESTRIN>(buffer[i], type, alpha);
}


} // namespace job::ai::comp
