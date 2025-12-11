#pragma once
#include <vector>
#include <cmath>
#include "learn_concept.h"
#include "runner.h"

namespace job::ai::learn {
class XORLearn {
public:
    XORLearn(threads::ThreadPool::Ptr pool = nullptr):
        m_pool(std::move(pool))
    {
        if(!m_pool)
            JOB_LOG_WARN("[XORLearn] has no pool");
    }

    [[nodiscard]] float learn(const evo::Genome &genome) const
    {
        infer::Runner runner(genome, m_pool, m_memSize);  // Might not be a good idea here. Might want to flywheel this.

        float totalError = 0.0f;

        for (const auto &sample : m_data) {
            const auto dim = static_cast<uint32_t>(sample.input.size());
            cords::ViewR::Extent inputShape{1u, dim};
            cords::ViewR inputView(const_cast<float*>(sample.input.data()), inputShape);

            auto output = runner.run(inputView);
            float val = output.data()[0];

            uint32_t bits;
            std::memcpy(&bits, &val, sizeof(float));
            if ((bits & 0x7F800000u) == 0x7F800000u)
                return -1e9f;

            float diff = val - sample.target[0];
            totalError += diff * diff;
        }

        float mse = totalError / 4.0f;
        return 1.0f / (1.0f + mse);
    }

    [[nodiscard]] int memSize(int memsize)
    {
        m_memSize = memsize;
        return m_memSize;
    }


private:
    struct XORSample {
        std::vector<float> input;
        std::vector<float> target;
    };

    std::vector<XORSample>      m_data = {
        {{0.f, 0.f}, {0.f}},
        {{0.f, 1.f}, {1.f}},
        {{1.f, 0.f}, {1.f}},
        {{1.f, 1.f}, {0.f}}
    };
    threads::ThreadPool::Ptr    m_pool = nullptr;
    int                         m_memSize{1};

};

}
