#pragma once

#include <concepts>
#include <string>

#include "genome.h"

namespace job::ai::learn {

// T_Learner is the Learner (e.g., XORLearn, CartPoleLearn, PortalLearn)
template <typename T_Learn>
concept Learn = requires(T_Learn t, const evo::Genome &genome, int memSize)
{
    // Must have an learn method that returns a float (fitness)
    { t.learn(genome) } -> std::convertible_to<float>;

    { t.name() } -> std::convertible_to<std::string>;

    { t.memSize(memSize) } -> std::convertible_to<int>;
};

} // namespace job::ai::learn
