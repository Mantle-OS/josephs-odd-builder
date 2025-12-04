#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

#include <job_thread_pool.h>
#include <job_parallel_for.h>
#include "gemm.h"
#include "activation_math.h"

namespace job::ai::comp {

// MLP / Feed-Forward Kernel | Activate(X * W1 + B1) * W2 + B2
//
// Arguments:
//   pool:        Thread pool for parallelization
//   B:           Batch Size (or Sequence Length if processing tokens)
//   d_model:     Input dimension
//   d_hidden:    Hidden dimension (usually 4 * d_model)
//   X:           Input [B x d_model]
//   W1:          Weights [d_model x d_hidden]
//   W2:          Weights [d_hidden x d_model]
//   HiddenBuf:   Intermediate buffer [B x d_hidden] (Caller provides memory!)
//   Out:         Output [B x d_model]
//   act:         Activation Function Type
inline void mlpForward(
    threads::ThreadPool &pool,
    int Batch,
    int d_model,
    int d_hidden,
    const core::real_t* __restrict__ X,
    const core::real_t* __restrict__ W1,
    const core::real_t* __restrict__ W2,
    core::real_t* __restrict__ HiddenBuf,
    core::real_t* __restrict__ Out,
    ActivationType act = ActivationType::GELU)
{
    // X [B, d_model] * W1 [d_model, d_hidden] -> HiddenBuf [B, d_hidden]
    // Alpha=1, Beta=0 (Overwrite HiddenBuf)
    sgemm_parallel_raw(pool, Batch, d_hidden, d_model,
                   core::real_t(1.0), X, d_model, W1,
                   d_hidden, core::real_t(0.0), HiddenBuf, d_hidden);

    // Activation: HiddenBuf = Activate(HiddenBuf)
    // This is memory-bound, parallelize to saturate bandwidth.
    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;

    // treat the buffer as a flat 1D array for activation
    threads::parallel_for(pool, size_t{0}, total_hidden, [&](size_t i) {
        // "activate" is the inline switch statement from activation_math.h. The compiler will likely inline the specific function if PGO is used, (Hopefully)
        HiddenBuf[i] = activate(act, HiddenBuf[i]);
    });

    // HiddenBuf [B, d_hidden] * W2 [d_hidden, d_model] -> Out [B, d_model]
    // Alpha=1, Beta=0 (Overwrite Out)
    // Note: In residual blocks (Add & Norm), Beta might be 1.0 to add to 'X'.... I think
    sgemm_parallel_raw(pool, Batch, d_model, d_hidden, core::real_t(1.0), HiddenBuf,
                   d_hidden, W2, d_model, core::real_t(0.0), Out, d_model);
}


// This "could" be better
// Gated MLP (SwiGLU / GeGLU)
// Implements: Out = (Activate(X * W1) * (X * V1)) * W2
// Requires 3 weight matrices.
inline void gatedMlpForward(
    job::threads::ThreadPool &pool,
    int Batch,
    int d_model,
    int d_hidden,
    const core::real_t* __restrict__ X,
    const core::real_t* __restrict__ W_gate, // W1
    const core::real_t* __restrict__ W_val,  // V1
    const core::real_t* __restrict__ W_proj, // W2
    core::real_t* __restrict__ GateBuf,      // [B x d_hidden]
    core::real_t* __restrict__ ValBuf,       // [B x d_hidden]
    core::real_t* __restrict__ Out,
    ActivationType act = ActivationType::SELU) // Swish
{
    // X * W_gate -> GateBuf
    sgemm_parallel_raw(pool, Batch, d_hidden, d_model, core::real_t(1.0), X,
                   d_model, W_gate, d_hidden, core::real_t(0.0), GateBuf, d_hidden);

    // X * W_val -> ValBuf
    sgemm_parallel_raw(pool, Batch, d_hidden, d_model, core::real_t(1.0), X,
                   d_model, W_val, d_hidden, core::real_t(0.0), ValBuf, d_hidden);

    // gatebuf = activate(gatebuf) * valbuf
    size_t total_hidden = static_cast<size_t>(Batch) * d_hidden;

    job::threads::parallel_for(pool, size_t{0}, total_hidden, [&](size_t i) {
        core::real_t g = activate(act, GateBuf[i]);
        GateBuf[i] = g * ValBuf[i];
    });

    // GateBuf * W_proj -> Out
    sgemm_parallel_raw(pool, Batch, d_model, d_hidden, core::real_t(1.0),
                   GateBuf, d_hidden, W_proj, d_model, core::real_t(0.0), Out, d_model);
}

} // namespace job::ai::comp
