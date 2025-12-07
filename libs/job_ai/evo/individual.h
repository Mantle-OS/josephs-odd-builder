#pragma once
#include "genome.h"

// Left overs that is not really needed anymore. . . .
namespace job::ai::evo {
struct Individual {
    Genome  genome;
    [[nodiscard]] float fitness() const
    {
        return genome.header.fitness;
    }

    void setFitness(float value)
    {
        genome.header.fitness = value;
    }
};
}
