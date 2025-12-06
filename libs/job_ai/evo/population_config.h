#pragma once
#include <cstddef>
#include "crossover_config.h"
#include "mutator_config.h"
namespace job::ai::evo {
struct PopulationConfig {
    std::size_t         size{100};
    bool                elitism{true};      // Keep best genome?
    size_t              eliteCount{1};      // How many elites?
    CrossoverConfig     crossover{};
    MutatorConfig       mutator{};
};
} // namespace job::ai::evo
