#pragma once

#include <cstddef>

#include "crossover_config.h"
#include "mutator_config.h"

namespace job::ai::evo {

struct PopulationConfig {
    std::size_t         populationSize{100};    // Number of individuals (Genomes) per generation.
    bool                elitism{true};          // If true, carry over the top N individuals unchanged into the next generation.
    std::size_t         eliteCount{1};          // Number of elites to preserve when elitism is enabled. Should satisfy: eliteCount <= populationSize.
    CrossoverConfig     crossover{};            // Configuration for crossover (how parents are combined into offspring).
    MutatorConfig       mutator{};              // Configuration for mutation (how offspring are perturbed).
};

} // namespace job::ai::evo
