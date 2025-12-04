#pragma once

#include <job_thread_pool.h>

#include "adapter_types.h"
#include "view.h"

namespace job::ai::adapters {

class IAdapter {
public:
    virtual ~IAdapter() = default;

    // Configuration
    [[nodiscard]] virtual AdapterType type() const = 0;

    // The Core Contract: "Compute Interactions"
    // Sources (Keys/Particles), Targets (Queries/Positions), Output (Potentials/Attention)
    // raw pointers/views here for maximum speed (The 32ns rule)
    virtual void compute(job::threads::ThreadPool &pool, const cords::ViewR &sources, const cords::ViewR &targets, cords::ViewR &output ) = 0;
};

} // namespace job::ai::adapters

