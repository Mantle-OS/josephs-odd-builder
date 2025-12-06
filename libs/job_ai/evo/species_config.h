#pragma once
namespace job::ai::evo {
struct SpeciesConfig {
    float compatibilityThreshold{3.0f};
    float excessCoeff{1.0f};
    float disjointCoeff{1.0f};
    float weightDiffCoeff{0.5f};
};
}
