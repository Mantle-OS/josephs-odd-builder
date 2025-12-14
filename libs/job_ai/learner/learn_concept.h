#pragma once

#include <concepts>
#include <string>
#include <cstdint>

namespace job::ai::learn {

// The "Contract" for any Config object passed to a Learner
template <typename T>
concept LearnerConfig = requires(T c) {
    { c.memSize } -> std::convertible_to<uint32_t>; // Workspace Limit
    { c.name }    -> std::convertible_to<std::string>;
};

// The "Contract" for the Learner itself
template <typename T, typename T_Genome, typename T_Config>
concept IsLearner =
    LearnerConfig<T_Config> &&
    requires(T t, const T_Genome& g, const T_Config& config)
{
    // 1. The Core Loop (Must return fitness float)
    //    We pass memSize explicitly to ensure the learner respects the budget
    { t.learn(g, config.memSize) } -> std::convertible_to<float>;

    // 2. Topology Requirements (Trainer needs these to build the network)
    { t.inputDim() }  -> std::convertible_to<uint32_t>;
    { t.outputDim() } -> std::convertible_to<uint32_t>;

    // 3. Configuration Injection
    //    The Trainer will call these to set up the Learner before running
    { t.setMemLimit(config.memSize) } -> std::same_as<void>;
    { t.setName(config.name) }        -> std::same_as<void>;
};

} // namespace job::ai::learn
