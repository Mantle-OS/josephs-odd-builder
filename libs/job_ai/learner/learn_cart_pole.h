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

namespace job::ai::learn {

class CartPoleLearn final : public ILearn {
public:
    explicit CartPoleLearn(threads::ThreadPool::Ptr pool = nullptr):
        m_pool(std::move(pool))
    {
        if(!m_pool) {
            // Warn or rely on runner to handle single-threaded?
            // Runner currently requires a pool ref, so this is critical.
        }
    }

    [[nodiscard]] float learn(const evo::Genome &genome, uint8_t wsMb = 1) override
    {
        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, wsMb);
        else
            m_runner->reload(genome);

        resetSim(12345);

        float fitness = 0.0f;
        const int max_steps = 500;

        for (int t = 0; t < max_steps; ++t) {
            float obs[4] = {m_x, m_x_dot, m_theta, m_theta_dot};
            cords::ViewR::Extent inputShape{1u, 4u};
            cords::ViewR inputView(obs, inputShape);

            auto output = m_runner->run(inputView, wsMb);

            const float* logits = output.data();

            // NaN Check
            uint32_t check;
            std::memcpy(&check, &logits[0], sizeof(float));
            if ((check & 0x7F800000u) == 0x7F800000u)
                return 0.0f;

            // 0 = Left, 1 = Right
            int action = (logits[1] > logits[0]) ? 1 : 0;

            // D. Step Physics
            if (!stepSim(action)) {
                break; // Died
            }
            fitness += 1.0f;
        }

        return fitness;
    }

    //
    [[nodiscard]] uint32_t inputDimension() const noexcept override
    {
        return 4;
    }

    // Left, Right
    [[nodiscard]] uint32_t outputDimension() const noexcept override
    {
        return 2;
    }

    [[nodiscard]] static std::unique_ptr<ILearn> create(threads::ThreadPool::Ptr pool)
    {
        return std::make_unique<CartPoleLearn>(std::move(pool));
    }

private:
    float                           m_x{0.0f};
    float                           m_x_dot{0.0f};
    float                           m_theta{0.0f};
    float                           m_theta_dot{0.0f};
    std::mt19937                    m_rng;
    threads::ThreadPool::Ptr        m_pool;
    std::unique_ptr<infer::Runner>  m_runner{nullptr};

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
