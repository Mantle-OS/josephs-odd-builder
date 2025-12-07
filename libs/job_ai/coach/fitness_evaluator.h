#pragma once

#include <vector>
#include <memory>
#include <span>

#include "runner.h"

namespace job::ai::coach {

struct TrainingSample {
    std::vector<float> input;  // context (c)
    std::vector<float> target; // intention (i)
};

class IFitnessEvaluator {
public:
    using Ptr = std::shared_ptr<IFitnessEvaluator>;
    [[nodiscard]] virtual std::string name() const = 0;

    virtual ~IFitnessEvaluator() = default;
    [[nodiscard]] virtual float evaluate(infer::Runner &runner, std::span<const TrainingSample> dataset) = 0;
};

} // namespace job::ai::coach
