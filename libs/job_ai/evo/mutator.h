#pragma once

#include <random>
#include <vector>
#include <cmath>
#include <algorithm>

#include <job_random.h>

#include "aligned_allocator.h"

namespace job::ai::evo {

class Mutator {
public:
    Mutator(uint64_t seed) :
        m_gen(seed)
    {

    }

    // x = x + N(0, sigma)
    // Optimized for SIMD-friendly vectors
    template <typename T>
    void perturb(std::vector<T, base::AlignedAllocator<T, 64>> &weights, core::real_t sigma)
    {
        std::normal_distribution<core::real_t> dist(core::real_t(0.0), sigma);
        for (auto& w : weights)
            w += dist(m_gen);
    }

    // "Swap" Mutation (Shuffle a range)
    template <typename T>
    void shuffleRange(std::vector<T, base::AlignedAllocator<T, 64>> &weights, size_t start, size_t len)
    {
        if (start + len <= weights.size())
            std::shuffle(weights.begin() + start, weights.begin() + start + len, m_gen);
    }

    // Structural Mutation: Insert a connection (Conceptual - modifies Genome arch)
    // Structural Mutation: Delete a connection

private:
    std::mt19937_64 m_gen;
};

} // namespace job::ai::evo

