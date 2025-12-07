#pragma once

#include <job_thread_pool.h>
#include "genome.h"
#include "ilayer.h"
#include "workspace.h"

namespace job::ai::infer {

class Runner {
    using Alloc = cords::AlignedAllocator<float, 64>;
public:
    Runner(const evo::Genome &genome, threads::ThreadPool::Ptr pool);
    cords::ViewR run(const cords::ViewR& input);
    void reset();

private:
    void buildNetwork();

    threads::ThreadPool::Ptr                        m_pool;
    evo::Genome                                     m_genome; // Stored copy
    std::vector<std::unique_ptr<layers::ILayer>>    m_layers;
    Workspace                                       m_workspace;
    std::vector<float, Alloc>                       m_bufA;
    std::vector<float, Alloc>                       m_bufB;
    size_t                                          m_maxLayerDim{0};
};
}
