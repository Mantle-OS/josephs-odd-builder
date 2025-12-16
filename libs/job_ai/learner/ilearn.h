#pragma once

#include <memory>

#include "genome.h"

namespace job::ai::learn {

class ILearn {
public:
    using Ptr   = std::shared_ptr<ILearn>;
    using UPtr  = std::unique_ptr<ILearn>;
    virtual ~ILearn() = default;

    // The Core Loop:
    // 1. Load Genome (Flywheel)
    // 2. Run Simulation (Physics + Inference)
    // 3. Return Fitness Score (Higher is better)
    [[nodiscard]] virtual float learn(const evo::Genome &genome)  = 0;

    // Optional: Returns the input dimension required by this environment
    [[nodiscard]] virtual uint32_t inputDimension() const noexcept = 0;

    // Optional: Returns the output/action dimension required
    [[nodiscard]] virtual uint32_t outputDimension() const noexcept = 0;

    virtual void onTokenTime([[maybe_unused]]uint64_t generation, [[maybe_unused]]uint64_t seed) {}
};

} // namespace job::ai::learner
