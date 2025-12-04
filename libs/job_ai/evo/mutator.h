#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "aligned_allocator.h"
#include "genome.h"

namespace job::ai::evo {

class Mutator {
public:
    explicit Mutator(uint64_t seed) :
        m_gen(seed)
    {

    }

    // Weight Mutation (Gaussian Perturbation)
    // Adds noise N(0, sigma) to all weights. This is the primary mechanism for Evolution Strategies (ES).
    void perturb(Genome& genome, float sigma) {
        std::normal_distribution<float> dist(0.0f, sigma);
        // mutate the contiguous weight blob directly.
        // Because aligned_allocator guarantees alignment, the compiler can autovectorize this loop if we are lucky.... or I guess I can explicit SIMD it later.
        for (float& w : genome.weights)
            w += dist(m_gen);
    }


    // Genetic Algorithm
    // Uniform Crossover: For each weight, flip a coin to pick from Parent A or B.
    [[nodiscard]] Genome crossover(const Genome& mom, const Genome& dad) {
        // Topology check: In simple NEAT, we align genes.
        // For v1 Fixed Topology, we assert sizes match.
        if (mom.weights.size() != dad.weights.size())
            return mom;

        Genome child = mom;
        std::uniform_int_distribution<int> coin(0, 1);

        const size_t count = mom.weights.size();
        for (size_t i = 0; i < count; ++i)
            if (coin(m_gen) == 1)
                child.weights[i] = dad.weights[i];

        child.header.parent_id = mom.header.uuid;
        // UUID generation needs to happen here or in the Coach. still figuing that part out
        return child;
    }

    // -------------------------------------------------------------------------
    // Topology Mutation (NEAT-lite)
    // -------------------------------------------------------------------------
    // These act on the `architecture` vector (LayerGenes).
    // Example: Randomly disable a layer (Dropout-like mutation)
    // Example: Add a random bypass connection (Residual)
    // (We can implement these as we flesh out the Layer types)

private:
    std::mt19937_64 m_gen;
};

} // namespace job::ai::evo
