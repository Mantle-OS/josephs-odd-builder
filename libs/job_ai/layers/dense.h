#pragma once

#include "ilayer.h"
#include "cords/matrix.h"
#include "cords/aligned_allocator.h"
#include "comp/gemm.h"
#include "comp/activation_math.h"
namespace job::ai::layers {

class Dense : public ILayer {
public:
    Dense(int inputs, int outputs, comp::ActivationType act) :
        m_inputs(inputs),
        m_outputs(outputs),
        m_act(act)
    {
        size_t weightCount = static_cast<size_t>(inputs) * outputs;
        size_t biasCount   = static_cast<size_t>(outputs);
        size_t totalParams = weightCount + biasCount;

        m_storage.resize(totalParams);

        m_weightsPtr = m_storage.data();
        m_biasPtr    = m_storage.data() + weightCount;
    }


    // Disable Copying to prevent dangling internal pointers
    Dense(const Dense&) = delete;
    Dense& operator=(const Dense&) = delete;

    // Move is safe (std::vector move preserves data address), but explicit is better
    Dense(Dense&& other) noexcept = default;
    Dense& operator=(Dense&& other) noexcept = default;



    [[nodiscard]] LayerType type() const override { return LayerType::Dense; }
    [[nodiscard]] std::string name() const override { return "Dense"; }

    void forward(job::threads::ThreadPool &pool, const cords::ViewR &input, cords::ViewR &output) override
    {
        cords::Matrix W(m_weightsPtr, m_inputs, m_outputs);
        cords::Matrix A(input.data(), input.extent()[0], input.extent()[1]);
        cords::Matrix C(output.data(), output.extent()[0], output.extent()[1]);

        comp::sgemm_parallel(pool, A, W, C);

        size_t batch = output.extent()[0];
        size_t cols  = output.extent()[1];
        size_t total = batch * cols;

        float *outData = output.data();
        const float* biasData = m_biasPtr;
        comp::ActivationType act = m_act;

        job::threads::parallel_for(pool, size_t{0}, total, [=](size_t i) {
            // Broadcasting: Bias index is column index
            int col = i % cols;
            float val = outData[i] + biasData[col];
            // Uses local 'act', no 'this->' indirection needed
            outData[i] = comp::activate(act, val);
        });
    }

    // Return flattened view of ALL parameters (Weights + Biases)
    // Used by the Mutator to perturb the layer in one go.
    [[nodiscard]] cords::ViewR parameters() override {
        // FIX: Explicitly construct Extents to resolve ambiguity
        // We return a 1D view (Fiber) of the whole storage blob
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_storage.data(), Ext(static_cast<uint32_t>(m_storage.size())));
    }

    [[nodiscard]] size_t parameterCount() const override {
        return m_storage.size();
    }

private:
    int m_inputs;
    int m_outputs;
    comp::ActivationType m_act;

    // Owned Memory (Aligned) - The "Blob"
    std::vector<float, cords::AlignedAllocator<float, 64>> m_storage;

    // Pointers into the Blob (fast access, valid as long as m_storage doesn't resize)
    float *m_weightsPtr{nullptr};
    float *m_biasPtr{nullptr};
};

} // namespace job::ai::layers
