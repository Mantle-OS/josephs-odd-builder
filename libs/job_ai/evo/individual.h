#pragma once
#include "genome.h"
namespace job::ai::evo {
struct Individual {
    Genome  genome;
    float   fitness{0.0f};
};
}
