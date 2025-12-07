#pragma once

namespace job::ai::evo {

struct SpeciesConfig {
    // Compatibility distance threshold:
    // if distance(g1, g2) < compatibilityThreshold -> same species
    // if distance(g1, g2) >= compatibilityThreshold -> different species
    float compatibilityThreshold{3.0f};

    // NEAT-style distance coefficients:
    // distance = (excessCoeff * E / N) +
    //            (disjointCoeff * D / N) +
    //            (weightDiffCoeff * W̄)
    //
    // E = number of excess genes
    // D = number of disjoint genes
    // N = normalization factor (e.g. max(genome1Size, genome2Size) or 1 if small)
    // W̄ = average weight difference of matching genes
    float excessCoeff{1.0f};
    float disjointCoeff{1.0f};
    float weightDiffCoeff{0.5f};
};

} // namespace job::ai::evo
