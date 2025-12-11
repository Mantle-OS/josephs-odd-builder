#include "ilayer.h"
#include "layer_types.h"
#include "gemm.h"

namespace job::ai::layers {


class LinearLoRA : public ILayer {


    LinearLoRA(size_t inputs, size_t outputs, size_t rank) :
        m_inputs(inputs),
        m_outputs(outputs),
        m_rank(rank)
    {
        m_storage.resize((m_rank * m_inputs) + (m_outputs * m_rank));
        // FIXME INIT
    }

    [[nodiscard]] LayerType type() const noexcept override
    {
        return LayerType::LinearLoRA;
    }

    [[nodiscard]] virtual const std::string &name() noexcept override
    {
        return m_LayerName;
    }

    [[nodiscard]] cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const override
    {
        return inputShape; // Identity shape
    }


    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR       &input,
                 cords::ViewR             &output,
                 infer::Workspace         &workspace) override
    {
        // Uses the shared frozen weights.
        // If m_frozenW is null, acts like standard low-rank network.
        if (m_frozenW) {
            cords::Matrix frozenView(m_frozenW->data(), cords::Extents({m_outputs, m_inputs}));
            comp::sgemm_parallel(pool, frozenView, input, output);
        } else {
            // Zero out output if no base
            std::fill(output.begin(), output.end(), 0.0f);
        }

        // 2. Adapter Path: Y += B * (A * X)

        // View A: [Rank x Inputs]
        float *ptrA = m_storage.data();
        cords::Matrix viewA(ptrA, cords::Extents({m_rank, m_inputs}));

        // View B: [Outputs x Rank]
        float* ptrB = ptrA + (m_rank * m_inputs);
        cords::Matrix viewB(ptrB, cords::Extents({m_outputs, m_rank}));

        // Temp Buffer for (A * X). Size = [Batch, Rank]
        // Rank is tiny (4), so this fits in L1 cache usually.
        size_t batchSize = input.extent()[0];
        float *tempPtr = workspace.alloc(batchSize * m_rank);
        cords::Matrix tempView(tempPtr, {batchSize, m_rank});

        // Down-project: Temp = A * X
        comp::sgemm_parallel(pool, viewA, input, tempView);

        // Up-project & Accumulate: Output += B * Temp
        // 'true' flag for accumulate/add-to-destination
        comp::sgemm_parallel(pool, viewB, tempView, output, true);
    }

    // ... getters ...


    [[nodiscard]] std::size_t parameterCount() const noexcept  override
    {
        return m_storage.size();
    }

    [[nodiscard]] cords::ViewR parameters() noexcept override
    {
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_storage.data(), Ext(static_cast<uint32_t>(m_storage.size())));
    }

    void resetState() noexcept override
    {
        // FIXME LATER
    };

    // Special method to load the Frozen W (shared across all runners)
    void setFrozenWeights(std::shared_ptr<std::vector<float>> weights)
    {
        // m_frozenW->clear();
        m_frozenW = weights;
    }

private:
    size_t m_inputs;
    size_t m_outputs;
    size_t m_rank;

    // The storage for A and B (Evolved)
    // std::vector<float> m_adapterParams;

    std::vector<float, cords::AlignedAllocator<float, 64>>  m_storage;



    // Pointer to the massive frozen weights (Shared)
    std::shared_ptr<std::vector<float>> m_frozenW;

    std::string m_LayerName{"LinearLoRA"};

};

}
