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
        alignas(32) float tmp[8] = {0}; // Ensure zero-pad
        for (int i = 0; i < count; ++i) tmp[i] = ptr[i];
        return SIMD::pull(tmp);
    }
}

// Helper: Masked Store (Reuse logic from gemm.h)
static inline void maskedStore(float* ptr, f32 val, int count) {
    if (count >= SIMD::width()) {
        SIMD::mov(ptr, val);
    } else {
        alignas(32) float tmp[8];
        SIMD::mov(tmp, val);
        for (int i = 0; i < count; ++i) ptr[i] = tmp[i];
    }
}

template <typename OP, bool UseBias>
void fusedDenseKernel(
    const float* __restrict__ A,       // Input [Rows x InFeatures]
    const float* __restrict__ W,       // Weights [InFeatures x OutFeatures]
    const float* __restrict__ B,       // Bias [OutFeatures]
    float* __restrict__ output,        // Output [Rows x OutFeatures]
    int rows,
    int in_features,
    int out_features,
    float alpha)
{
    constexpr int Step = SIMD::width(); // 8 for AVX2

    for (int r = 0; r < rows; ++r) {
        const float *row_input = A + (r * in_features);
        float *row_out = output + (r * out_features);

        int c = 0;
        for (; c <= out_features - Step; c += Step) {

            // initialize accumulator (Bias or Zero)
            auto acc = (UseBias) ? SIMD::pull(B + c) : SIMD::zero();

            // dot product - fused register
            for (int k = 0; k < in_features; ++k) {
                // input scalar: A[r, k]
                auto in_val = SIMD::set1(row_input[k]);

                // load weight vector: W[k, c...c+7]
                auto w_vec = SIMD::pull(W + k * out_features + c);

                // FMA: acc += in * w
                acc = SIMD::mul_plus(in_val, w_vec, acc);
            }

            // activation while still in register -> fast exp_estrin/schraudolph logic
            acc = OP::apply(acc, alpha);

            // write to memory
            SIMD::mov(row_out + c, acc);
        }

        if (c < out_features) {
            int rem = out_features - c;

            // masked bias load
            auto acc = (UseBias) ? maskedLoad(B + c, rem) : SIMD::zero();

            // masked dot product
            for (int k = 0; k < in_features; ++k) {
                auto in_val = SIMD::set1(row_input[k]);

                // Masked Weight Load (Safety Dance first lol!)
                auto w_vec = maskedLoad(W + k * out_features + c, rem);
                acc = SIMD::mul_plus(in_val, w_vec, acc);
            }

            // fast activation
            acc = OP::apply(acc, alpha);

            // masked store
            maskedStore(row_out + c, acc, rem);
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
