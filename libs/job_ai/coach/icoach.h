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

    using Evaluator = std::function<float(const evo::Genome&)>; // TODO this should not be a function and should come from its own module "eval" or "training"

    virtual void setPopulationSize(size_t size) = 0;
    virtual void setMutationRate(float rate) = 0;
    virtual void setMode(OptimizationMode mode) = 0;

    [[nodiscard]] virtual evo::Genome coach(const evo::Genome &parent, Evaluator eval) = 0;

    [[nodiscard]] virtual size_t generation() const = 0;
    [[nodiscard]] virtual float currentBestFitness() const = 0;
};

} // namespace job::ai::coach
