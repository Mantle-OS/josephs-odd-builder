#pragma once

#include <job_thread_pool.h>

#include <layer_types.h>
#include <activation_math.h>
#include <gemm.h>

namespace job::ai::machine {

class DenseLayer {
public:
    DenseLayer(int inputs, int outputs, base::ActivationType act) :
        m_inputs(inputs),
        m_outputs(outputs),
        m_act(act)
    {

    }

    // Forward ONLY. No cache. No Autograd graph.
    // X: [Batch, Inputs]
    // W: [Inputs, Outputs]
    // B: [Outputs]
    // Out: [Batch, Outputs]
    void forward(job::threads::ThreadPool &pool, int batch_size, const core::real_t *x, const core::real_t *w, const core::real_t* bias, core::real_t *out)
    {
        using namespace job::ai::base;
        // Linear: Out = X * W
        // Beta=0 (overwrite Out)
        sgemm_parallel(pool, batch_size, m_outputs, m_inputs,
                       1.0f, x, m_inputs, w, m_outputs, 0.0f, Out, m_outputs);

        // Bias Add + Activation "Fused" loop for memory efficiency
        size_t total = static_cast<size_t>(batch_size) * m_outputs;

        job::threads::parallel_for(pool, size_t{0}, total, [&](size_t idx) {
            // Row-major index -> which output neuron?
            int col = idx % m_outputs;

            core::real_t val = out[idx] + bias[col];
            out[idx] = activate(m_act, val);
        });
    }

private:
    int m_inputs;
    int m_outputs;
    base::ActivationType m_act;
};

} // namespace
