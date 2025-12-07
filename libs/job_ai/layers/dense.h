#pragma once

#include "ilayer.h"
#include "matrix.h"
#include "aligned_allocator.h"
#include "gemm.h"
#include "activation_math.h"
#include "activate_parallel.h"

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

        m_storage.resize(weightCount + biasCount);
        m_weightsPtr = m_storage.data();
        m_biasPtr    = m_storage.data() + weightCount;
    }

    Dense(const Dense&) = delete;
    Dense& operator=(const Dense&) = delete;
    Dense(Dense&&) noexcept = default;
    Dense& operator=(Dense&&) noexcept = default;

    [[nodiscard]] LayerType type() const override
    {
        return LayerType::Dense;
    }
    [[nodiscard]] std::string name() const override
    {
        return "Dense";
    }

    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output,
                 [[maybe_unused]] infer::Workspace &ws) override
    {
        size_t rows = input.extent()[0];
        size_t inFeatures = 0;

        if (input.rank() == 3) {
            rows *= input.extent()[1];     // Flatten Batch * Seq
            inFeatures = input.extent()[2];
        } else {
            inFeatures = input.extent()[1];
        }

        size_t approxFlops = rows * static_cast<size_t>(m_inputs) * static_cast<size_t>(m_outputs);
        constexpr size_t kMinFlopsForParallel = 16384;

        // Prepare Matrices
        cords::Matrix A(input.data(), rows, inFeatures);
        cords::Matrix W(m_weightsPtr, m_inputs, m_outputs);
        cords::Matrix C(output.data(), rows, m_outputs);

        if (approxFlops < kMinFlopsForParallel) {
            comp::sgemm(A, W, C);

            float *outData = output.data();
            const float *biasData = m_biasPtr;
            for (size_t i = 0; i < rows; ++i) {
                float* row_ptr = outData + (i * m_outputs);
                for (size_t j = 0; j < (size_t)m_outputs; ++j) {
                    // Bias
                    row_ptr[j] += biasData[j];
                    // Activation (Using the correct function from your header)
                    row_ptr[j] = comp::activate(m_act, row_ptr[j]);
                }
            }
            return ;
        } else {
            comp::sgemm_parallel(pool, A, W, C);

            float *outData = output.data();
            const float *biasData = m_biasPtr;
            size_t outDim = m_outputs;

            // Parallel Bias
            job::threads::parallel_for(pool, size_t{0}, rows, [=](size_t row_idx) {
                float* row_ptr = outData + (row_idx * outDim);
                using SIMD = job::ai::comp::SIMD;
                constexpr int K = SIMD::width();

                size_t j = 0;
                for (; j + (K-1) < outDim; j += K) {
                    auto v = SIMD::add(SIMD::pull(row_ptr + j), SIMD::pull(biasData + j));
                    SIMD::mov(row_ptr + j, v);
                }
                for (; j < outDim; ++j)
                    row_ptr[j] += biasData[j];
            });
        }
    }

    [[nodiscard]] cords::ViewR parameters() override
    {
        return cords::ViewR(m_storage.data(), cords::ViewR::Extent((uint32_t)m_storage.size()));
    }

    [[nodiscard]] size_t parameterCount() const override
    {
        return m_storage.size();
    }

    cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent& inputShape) const override
    {
        if (inputShape.rank() == 3)
            return { inputShape[0], inputShape[1], static_cast<uint32_t>(m_outputs) };

        return { inputShape[0], static_cast<uint32_t>(m_outputs) };
    }

private:
    int                                                     m_inputs;
    int                                                     m_outputs;
    comp::ActivationType                                    m_act;
    std::vector<float, cords::AlignedAllocator<float, 64>>  m_storage;
    float                                                   *m_weightsPtr{nullptr};
    float                                                   *m_biasPtr{nullptr};
};

} // namespace job::ai::layers
