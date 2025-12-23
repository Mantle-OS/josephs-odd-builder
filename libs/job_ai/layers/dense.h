#pragma once

#include <vector>

#include <simd_provider.h>

#include "activation.h"
#include "abstract_layer.h"
#include "layer_config.h"
#include "matrix.h"
#include "aligned_allocator.h"


namespace job::ai::layers {

class DenseLayer final : public AbstractLayer {
public:
    DenseLayer(uint32_t input, uint32_t output, LayerConfig cfg = LayerPresets::DenseConfig(), float alpha = 0.0f) :
        AbstractLayer(cfg, alpha)
    {
        m_cfg.inputs = input;
        m_cfg.outputs = output;

        const std::size_t weightCount = size_t(m_cfg.inputs) * m_cfg.outputs;
        const std::size_t biasCount   = m_cfg.hasBias() ? size_t(m_cfg.outputs) : 0;

        m_storage.resize(weightCount + biasCount);
        m_weightsPtr = m_storage.data();
        m_biasPtr = biasCount ? (m_storage.data() + weightCount) : nullptr;
    }

    DenseLayer(const DenseLayer&) = delete;
    DenseLayer& operator=(const DenseLayer&) = delete;
    DenseLayer(DenseLayer&&) noexcept = default;
    DenseLayer& operator=(DenseLayer&&) noexcept = default;


    //////////////////////////////////////////////
    // AbstractLayer override
    //////////////////////////////////////////////
    [[nodiscard]] LayerType type() const noexcept override final
    {
        return m_cfg.type;
    }

    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output,
                 [[maybe_unused]]infer::Workspace &workspace) override
    {
        assert(isCompactRowMajor(input) && "DenseLayer: non-compact input");

        std::size_t rows = 0;
        std::size_t inFeatures = 0;
        inferDenseShape(input, rows, inFeatures);

        assert(inFeatures == static_cast<std::size_t>(m_cfg.inputs));

        // Pointers
        const float* A_ptr = input.data();
        const float* W_ptr = m_weightsPtr;
        const float* B_ptr = m_cfg.hasBias() ? m_biasPtr : nullptr;
        float* O_ptr = output.data();

        // Dimensions
        int M = static_cast<int>(rows);
        int K = static_cast<int>(inFeatures);
        int N = static_cast<int>(m_cfg.outputs);

        // Heuristic
        const std::size_t flops = rows * inFeatures * static_cast<std::size_t>(m_cfg.outputs);
        constexpr std::size_t kMinFlopsForParallel = 32768; // Bumped up slightly for overhead

        bool runParallel = (rows > 1) && (flops > kMinFlopsForParallel);

        if (runParallel) {
            comp::activateDenseParallel<false>(
                pool,
                A_ptr, W_ptr, B_ptr, O_ptr,
                M, K, N,
                m_cfg.activation, m_cfg.hasBias(), m_alpha
                );
        } else {
            comp::activateDense<false>(
                A_ptr, W_ptr, B_ptr, O_ptr,
                M, K, N,
                m_cfg.activation, m_cfg.hasBias(), m_alpha
                );
        }
    }
    [[nodiscard]] cords::ViewR parameters() noexcept override
    {
        return cords::ViewR(m_storage.data(), cords::ViewR::Extent((uint32_t)m_storage.size()));
    }

    void resetState() noexcept override
    {
        // FIXME LATER Not now.
    };

    // FLY WHEEL
    void loadWeights(float *weights) override
    {
        m_weightsPtr = weights;
        if (m_cfg.hasBias()) {
            const size_t offset = size_t(m_cfg.inputs) * m_cfg.outputs;
            m_biasPtr = weights + offset;
        } else {
            m_biasPtr = nullptr;
        }
    }

    // Dense is in-place / direct-write
    [[nodiscard]] size_t scratchSize(const cords::ViewR::Extent &) const override
    {
        return 0;
    }

    [[nodiscard]] cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const override
    {
        if (inputShape.rank() == 3)
            return { inputShape[0], inputShape[1], static_cast<uint32_t>(m_cfg.outputs) };

        if (inputShape.rank() == 2)
            return { inputShape[0], static_cast<uint32_t>(m_cfg.outputs) };

        // rank == 1 -> treat as [1, D]
        return { 1u, static_cast<uint32_t>(m_cfg.outputs) };
    }

private:

    // static inline void addBiasRow(float *row, const float *bias, size_t count)
    // {
    //     using SIMD = job::ai::comp::SIMD;
    //     constexpr int K = SIMD::width();

    //     size_t j = 0;
    //     for (; j + K <= count; j += K) {
    //         auto v = SIMD::add(SIMD::pull(row + j), SIMD::pull(bias + j));
    //         SIMD::mov(row + j, v);
    //     }
    //     for (; j < count; ++j)
    //         row[j] += bias[j];
    // }

    // void handleBiasAndActivation(job::threads::ThreadPool &pool, float *outData,
    //                              size_t rows, size_t cols, bool parallel)
    // {
    //     if (m_cfg.hasBias()) {
    //         float *biasPtr = m_biasPtr;
    //         if (parallel) {
    //             job::threads::parallel_for(pool, size_t{0}, rows, [outData, biasPtr, cols](size_t i) {
    //                 addBiasRow(outData + i*cols, biasPtr, cols);
    //             });
    //         } else {
    //             for(size_t i=0; i<rows; ++i)
    //                 addBiasRow(outData + i*cols, biasPtr, cols);
    //         }
    //     }

    //     const size_t totalElements = rows * cols;

    //     if (parallel) {
    //         comp::activate_buffer(pool, outData, totalElements, m_cfg.activation, m_alpha);
    //     } else {
    //         // Fallback to serial activation to prevent starvation
    //         comp::activate_buffer_serial(outData, totalElements, m_cfg.activation, m_alpha);
    //     }

    //     // RUN FOREST RUN !!!!@!@!@!
    //     // comp::activate_buffer(pool, outData, rows * cols, m_cfg.activation, m_alpha);
    // }

    std::vector<float, cords::AlignedAllocator<float, 64>>      m_storage;
    float                                                       *m_weightsPtr{nullptr};
    float                                                       *m_biasPtr{nullptr};
};

} // namespace job::ai::layers
