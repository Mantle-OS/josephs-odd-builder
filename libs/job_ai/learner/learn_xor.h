#pragma once
#include <vector>
#include <cmath>
#include <memory>
#include <cstring>

// INTERFACES
#include "ilearn.h" // Assuming you still want the OOP interface, otherwise remove
#include "infer/runner.h"

namespace job::ai::learn {

class XORLearn final : public ILearn {
public:
    explicit XORLearn(threads::ThreadPool::Ptr pool = nullptr) :
        m_pool(std::move(pool))
    {
        if(!m_pool) {
            // In a real system, maybe create a default local pool?
            // JOB_LOG_WARN("[XORLearn] has no pool");
        }
    }

    [[nodiscard]] float learn(const evo::Genome &genome, uint8_t wsMb = 1) override
    {
        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, wsMb);
        else
            m_runner->reload(genome);


        float totalError = 0.0f;

        for (const auto &sample : m_data) {
            // Wrap raw data in a View (Zero Copy)
            // XOR is 2D input
            cords::ViewR::Extent inputShape{1u, 2u};
            cords::ViewR inputView(const_cast<float*>(sample.input.data()), inputShape);

            // Run Inference (Reusing workspace)
            auto output = m_runner->run(inputView, wsMb);
            float val = output.data()[0];

            // Nan/Inf Check (Bitwise hack is fast)
            uint32_t bits;
            std::memcpy(&bits, &val, sizeof(float));
            if ((bits & 0x7F800000u) == 0x7F800000u)
                return -1e9f; // Punish instability heavily

            float diff = val - sample.target[0];
            totalError += diff * diff;
        }

        float mse = totalError / 4.0f;
        // Fitness: Higher is better (1.0 = perfect, 0.0 = terrible)
        return 1.0f / (1.0f + mse);
    }

    [[nodiscard]] uint32_t inputDimension() const noexcept override
    {
        return 2;
    }

    [[nodiscard]] uint32_t outputDimension() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] static std::unique_ptr<ILearn> create(threads::ThreadPool::Ptr pool)
    {
        return std::make_unique<XORLearn>(std::move(pool));
    }
private:
    struct XORSample {
        std::vector<float> input;
        std::vector<float> target;
    };

    // Hardcoded XOR Truth Table
    std::vector<XORSample> m_data = {
        {{0.f, 0.f}, {0.f}},
        {{0.f, 1.f}, {1.f}},
        {{1.f, 0.f}, {1.f}},
        {{1.f, 1.f}, {0.f}}
    };

    threads::ThreadPool::Ptr       m_pool;

    // The Persistent Engine
    std::unique_ptr<infer::Runner> m_runner{nullptr};
};

} // namespace job::ai::learn
