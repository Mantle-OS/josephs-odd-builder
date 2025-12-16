#pragma once
#include <vector>
#include <cmath>
#include <memory>
#include <cstring>

// CRYPTO
#include <job_random.h>

// INFR
#include "runner.h"

// LEARN
#include "ilearn.h"
#include "learn_config.h"
namespace job::ai::learn {

class CartPoleLearn final : public ILearn {
public:
    explicit CartPoleLearn(const LearnConfig &cfg = LearnPresets::CartPoleConfig(), threads::ThreadPool::Ptr pool = nullptr):
        m_pool(std::move(pool)),
        m_cfg(cfg)
    {
        if(!m_pool)
            JOB_LOG_WARN("[CartPoleLearn] Has no pool that is not good");
    }

    [[nodiscard]] static std::unique_ptr<ILearn> create(const LearnConfig &cfg ,threads::ThreadPool::Ptr pool)
    {
        return std::make_unique<CartPoleLearn>(cfg, std::move(pool));
    }

    [[nodiscard]] float learn(const evo::Genome &genome) override
    {
        // Use Config for Workspace
        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, m_cfg.initWsMb);
        else
            m_runner->reload(genome);

        // Use Config for Seed
        resetSim(m_cfg.seed);

        float stepsSurvived = 0.0f;

        // Use Config for Duration
        for (uint32_t t = 0; t < m_cfg.maxSteps; ++t) {
            float obs[4] = {m_x, m_x_dot, m_theta, m_theta_dot};

            // Input: Batch=1, Sequence=4 (Flat Observation)
            cords::ViewR inputView(obs, cords::makeBS(1u, 4u));

            auto output = m_runner->run(inputView, m_cfg.initWsMb);
            const float *logits = output.data();

            // Physics Failure = 0 Score (Not -1e9, just failure)
            if(punishLearner(logits[0]))
                return 0.0f;

            int action = (logits[1] > logits[0]) ? 1 : 0;

            if (!stepSim(action))
                break; // Died

            stepsSurvived += 1.0f;
        }

        // SCORING: Normalize to Target
        // If maxSteps=500 and target=100...
        // 250 steps -> (250/500)*100 = 50.0
        if (m_cfg.maxSteps == 0)
            return 0.0f;

        return (stepsSurvived / static_cast<float>(m_cfg.maxSteps)) * m_cfg.targetFitness;
    }

    [[nodiscard]] uint32_t inputDimension() const noexcept override
    {
        return 4;
    }

    // Left, Right
    [[nodiscard]] uint32_t outputDimension() const noexcept override
    {
        return 2;
    }

private:
    float                           m_x{0.0f};
    float                           m_x_dot{0.0f};
    float                           m_theta{0.0f};
    float                           m_theta_dot{0.0f};
    std::mt19937                    m_rng;
    threads::ThreadPool::Ptr        m_pool;
    std::unique_ptr<infer::Runner>  m_runner{nullptr};
    LearnConfig                     m_cfg;

    void resetSim(uint64_t seed)
    {
        m_rng.seed(seed);
        // Use local distributions
        std::uniform_real_distribution<float> dist(-0.05f, 0.05f);\
        m_x         = dist(m_rng);
        m_x_dot     = dist(m_rng);
        m_theta     = dist(m_rng);
        m_theta_dot = dist(m_rng);
    }

    bool stepSim(int action)
    {
        const float gravity = 9.8f;
        const float masscart = 1.0f;
        const float masspole = 0.1f;
        // FIXED: Calculated dynamically so masscart is used
        const float total_mass = masscart + masspole;
        const float length = 0.5f; // half-length
        const float polemass_length = masspole * length;
        const float force_mag = 10.0f;
        const float tau = 0.02f;

        float force = (action == 1) ? force_mag : -force_mag;
        float costheta = std::cos(m_theta);
        float sintheta = std::sin(m_theta);

        float temp = (force + polemass_length * m_theta_dot * m_theta_dot * sintheta) / total_mass;
        float thetaacc = (gravity * sintheta - costheta * temp) /
                         (length * (4.0f / 3.0f - masspole * costheta * costheta / total_mass));
        float xacc = temp - polemass_length * thetaacc * costheta / total_mass;

        m_x         += tau * m_x_dot;
        m_x_dot     += tau * xacc;
        m_theta     += tau * m_theta_dot;
        m_theta_dot += tau * thetaacc;

        // Fail conditions:
        // x > 2.4 units (Edge of screen)
        // theta > 12 degrees (~0.209 radians)
        if (m_x < -2.4f || m_x > 2.4f)
            return false;

        if (m_theta < -0.2094f || m_theta > 0.2094f)
            return false;

        return true;
    }
};

} // namespace job::ai::learn
