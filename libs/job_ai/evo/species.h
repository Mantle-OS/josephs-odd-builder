#pragma once
#include <cstddef>
#include <vector>
namespace job::ai::evo {
struct Species {
    std::size_t                 id;
    std::vector<std::size_t>    members; // indices into Population
    float                       bestFitness{0.0f};
    float                       age{0.0f};
    // FIXME Representative genome for distance calc
};
}
