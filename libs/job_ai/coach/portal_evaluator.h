#pragma once

#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

#include <job_logger.h>

#include <job_thread_pool.h>

#include "runner.h"
#include "genome.h"

namespace job::ai::coach {

struct TrainingSample {
    std::vector<float> input;
    std::vector<float> target;
};

class PortalEvaluator {
public:
    PortalEvaluator(threads::ThreadPool::Ptr pool) :
        m_pool(std::move(pool))
    {
        if(!m_pool)
            JOB_LOG_WARN("[PortalEvaluator] No pool for the Portal eq not good !");
    }

    void setDataset(std::vector<TrainingSample> data)
    {
        m_dataset = std::move(data);
    }

    // returns a function compatible with icoach -> evaluator
    std::function<float(const evo::Genome&)> portal()
    {
        return [this](const evo::Genome& candidate) -> float {
            return this->evaluate(candidate);
        };
    }

private:
    threads::ThreadPool::Ptr        m_pool;
    std::vector<TrainingSample>     m_dataset;

    float evaluate(const evo::Genome &genome)
    {
        if (m_dataset.empty())
            return 0.0f;

        infer::Runner runner(genome, m_pool);
        float totalPotential = 0.0f;

        for (const auto &sample : m_dataset) {
            // xor just for tonight
            cords::ViewR::Extent inputShape{1, 1, (uint32_t)sample.input.size()};
            cords::ViewR inputView(const_cast<float*>(sample.input.data()), inputShape);
            cords::ViewR output = runner.run(inputView);

            const float* outData = output.data();
            const float* tgtData = sample.target.data();
            size_t len = std::min(output.size(), sample.target.size());

            float deviation = 0.0f;

            for (size_t i = 0; i < len; ++i) {
                float val = outData[i];
                float diff = val - tgtData[i];
                deviation += diff * diff;

                // bitwise singularity check (nan/inf)
                uint32_t bits;
                std::memcpy(&bits, &val, 4);
                if ((bits & 0x7F800000) == 0x7F800000)
                    return -1e9f; // event horizon
            }
            totalPotential += deviation;
        }

        float meanPotential = totalPotential / m_dataset.size();
        return 1.0f / (1.0f + meanPotential);
    }
};

} // namespace job::ai::coach
