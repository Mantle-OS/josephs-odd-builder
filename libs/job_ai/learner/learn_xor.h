#pragma once
#include <vector>
#include <cmath>
#include <memory>
#include <cstring>

// INTERFACES
#include "infer/runner.h"

#include "ilearn.h"
#include "learn_config.h"
namespace job::ai::learn {

class XORLearn final : public ILearn {
public:
    explicit XORLearn(const LearnConfig &cfg = LearnPresets::XORConfig(), threads::ThreadPool::Ptr pool = nullptr) :
        m_pool(std::move(pool)),
        m_cfg{cfg}
    {
        if(!m_pool)
            JOB_LOG_WARN("[XORLearn] has no pool");
    }

    [[nodiscard]] static std::unique_ptr<ILearn> create(const LearnConfig &cfg, threads::ThreadPool::Ptr pool)
    {
        return std::make_unique<XORLearn>(cfg, std::move(pool));
    }

    [[nodiscard]] float learn(const evo::Genome &genome) override
    {
        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, m_cfg.initWsMb);
        else
            m_runner->reload(genome);

        float totalError = 0.0f;
        for (const auto &sample : m_data) {
            cords::ViewR inputView(const_cast<float*>(sample.input.data()),
                                   cords::makeBS(1u, 2u));

            auto output = m_runner->run(inputView, m_cfg.initWsMb);
            float val = output.data()[0];

            if(punishLearner(val))
                return -1e9f;

            float diff = val - sample.target[0];
            totalError += diff * diff;
        }

        float mse = totalError / 4.0f;
        return m_cfg.targetFitness / (1.0f + mse);
    }

    [[nodiscard]] uint32_t inputDimension() const noexcept override
    {
        return 2;
    }

    [[nodiscard]] uint32_t outputDimension() const noexcept override
    {
        return 1;
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
    LearnConfig                    m_cfg;
    std::unique_ptr<infer::Runner> m_runner{nullptr};
};

} // namespace job::ai::learn
