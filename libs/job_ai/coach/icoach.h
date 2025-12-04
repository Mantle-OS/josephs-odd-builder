#pragma once

#include <functional>
#include <vector>
#include <string>

#include "coach_types.h"
#include "genome.h"

namespace job::ai::coach {

class ICoach {
public:
    virtual ~ICoach() = default;

    // Identity
    [[nodiscard]] virtual CoachType type() const = 0;
    [[nodiscard]] virtual std::string name() const = 0;

    // -------------------------------------------------------------------------
    // The Fitness Function Contract
    // -------------------------------------------------------------------------
    // The user provides this lambda.
    // Input: A candidate Genome (The model to test).
    // Output: A float score (Fitness).
    using Evaluator = std::function<float(const evo::Genome&)>;

    // -------------------------------------------------------------------------
    // Configuration (Virtual Setters)
    // -------------------------------------------------------------------------
    // We use virtual setters so the Application doesn't need to know
    // the specific template type of the Coach implementation.
    virtual void setPopulationSize(size_t size) = 0;
    virtual void setMutationRate(float rate) = 0;
    virtual void setMode(OptimizationMode mode) = 0;

    // -------------------------------------------------------------------------
    // The Main Loop Step
    // -------------------------------------------------------------------------
    // 1. Takes the current best "Parent".
    // 2. Generates 'PopulationSize' mutants/offspring.
    // 3. Runs 'eval' on all of them (Parallelized internally via ThreadPool).
    // 4. Selects the winner.
    // 5. Returns the new best Genome for the next generation.
    [[nodiscard]] virtual evo::Genome step(const evo::Genome &parent, Evaluator eval) = 0;

    // -------------------------------------------------------------------------
    // Introspection
    // -------------------------------------------------------------------------
    [[nodiscard]] virtual size_t generation() const = 0;
    [[nodiscard]] virtual float currentBestFitness() const = 0;
};

} // namespace job::ai::coach
