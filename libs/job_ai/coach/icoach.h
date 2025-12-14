#pragma once

#include <functional>
#include <vector>
#include <string>

#include "coach_types.h"
#include "ilearn.h"
#include "genome.h"

namespace job::ai::coach {

class ICoach {
public:
    virtual ~ICoach() = default;

    // Identity
    [[nodiscard]] virtual CoachType type() const noexcept = 0;
    [[nodiscard]] virtual std::string &name() noexcept = 0;

    virtual void setPopulationSize(size_t size) noexcept = 0;
    virtual void setMutationRate(float rate) noexcept = 0;
    virtual void setMode(OptimizationMode mode) noexcept = 0;

    [[nodiscard]] virtual evo::Genome coach(const evo::Genome &parent) = 0;

    [[nodiscard]] virtual size_t generation() const noexcept = 0;
    [[nodiscard]] virtual float currentBestFitness() const noexcept = 0;

    [[nodiscard]] virtual learn::ILearn* learner() = 0;
};

} // namespace job::ai::coach
