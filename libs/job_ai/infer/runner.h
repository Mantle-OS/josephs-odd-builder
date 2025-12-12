#pragma once

#include <job_thread_pool.h>
#include "abstract_layer.h"
#include "genome.h"
#include "ilayer.h"
#include "workspace.h"

namespace job::ai::infer {

class Runner {
    using Alloc = cords::AlignedAllocator<float, 64>;
public:
    Runner(const evo::Genome &genome, threads::ThreadPool::Ptr pool , size_t workspaceSizeMB = 256);
    cords::ViewR run(const cords::ViewR& input);
    void reset();

    [[nodiscard]] bool isCompatible(const evo::Genome& g) const
    {
        size_t totalParams = 0;
        for(const auto& layer : m_layers)
            totalParams += layer->parameterCount();
        return g.weights.size() == totalParams;
    }

    // void reload(const evo::Genome &genome)
    // {
    //     if (isCompatible(genome)) {
    //         const float* rawData = genome.weights.data();
    //         size_t offset = 0;

    //         for (auto& layer : m_layers) {
    //             layer->loadWeights(rawData + offset);
    //             offset += layer->parameterCount();
    //         }
    //         m_genome = genome;
    //     } else {
    //         m_genome = genome;
    //         buildNetwork();
    //     }
    // }


private:
    void buildNetwork();

    threads::ThreadPool::Ptr                                m_pool;
    evo::Genome                                             m_genome; // Stored copy
    std::vector<std::unique_ptr<layers::AbstractLayer>>     m_layers;
    Workspace                                               m_workspace;
    std::vector<float, Alloc>                               m_bufA;
    std::vector<float, Alloc>                               m_bufB;
    size_t                                                  m_maxLayerDim{0};
};
}
