#pragma once
#include <vector>
#include "population.h"
#include "species_config.h"
#include "species.h"
namespace job::ai::evo {
class SpeciationEngine {
public:
    explicit SpeciationEngine(const SpeciesConfig &cfg) : m_cfg(cfg) {}


    void respeciate(const Population &pop)
    {
        // FIXME: Implement NEAT speciation logic
        // 1. Clear members
        // 2. For each genome, find compatible species
        // 3. If none, create new species
        // 4. Prune empty species
        (void)pop;
    }

    const std::vector<Species> &species() const { return m_species; }

private:
    SpeciesConfig           m_cfg;
    std::vector<Species>    m_species;

    // float compatibilityDistance(const Genome &a, const Genome &b) const;
};

} // namespace job::ai::evo
