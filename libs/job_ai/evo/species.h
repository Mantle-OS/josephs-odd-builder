#pragma once

#include <cstddef>
#include <limits>
#include <vector>

namespace job::ai::evo {

struct Species {
    std::size_t                 id{0};                                                  // Species identifier (assigned by coach / speciation engine)
    std::vector<std::size_t>    members;                                                // Indices into the population vector
    float                       bestFitness{std::numeric_limits<float>::lowest()};      // Best fitness observed among members this generation or across history
    float                       age{0.0f};                                              // Age in generations (incremented by the coach/speciation engine)

    // Representative for compatibility distance:
    // index into `members`, or index into the global population, depending on your convention.
    // For now, use `std::numeric_limits<std::size_t>::max()` as "no representative".
    std::size_t representative{std::numeric_limits<std::size_t>::max()};
};

} // namespace job::ai::evo
