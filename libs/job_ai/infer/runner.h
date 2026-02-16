#pragma once

// THREADS
#include <job_thread_pool.h>

// LAYER
#include "abstract_layer.h"

// EVO
#include "genome.h"

// CACHE
#include "workspace.h"

namespace job::ai::infer {

class Runner {
    using Alloc = core::AlignedAllocator<float, 64>; // Might now be needed anymore see notes on m_buf[A,B]
public:
    using Ptr = std::shared_ptr<Runner>;
    Runner(const evo::Genome &genome, threads::ThreadPool::Ptr pool, uint8_t initialWsMB = 1);  // Create the runner
    cords::ViewR run(const cords::ViewR &input, uint8_t wsMB = 1);                   // Start the whole process
    void reset();                                                                   // reset the runner
    [[nodiscard]] bool isCompatible(const evo::Genome &g) const;                    // well
    void reload(const evo::Genome &genome);                                         // flywheel


private:
    void buildNetwork();                                                        // build the network
    threads::ThreadPool::Ptr                                m_pool;             // The thread pool
    evo::Genome                                             m_genome;           // Stored copy
    std::vector<std::unique_ptr<layers::AbstractLayer>>     m_layers;           // The AbstractLayer
    Workspace                                               m_workspace;        // The cache
    std::vector<float, Alloc>                               m_bufA;             // We could just get this from the work space now ?
    std::vector<float, Alloc>                               m_bufB;             // We could just get this from the work space now ?
    size_t                                                  m_maxLayerDim{0};   // make geter and setter
};
}
