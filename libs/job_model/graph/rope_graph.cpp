#include "graph/rope_graph.h"

#include <limits>
#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr RopeGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                             const ggml::JobGgmlTensor &positions,
                                             uint32_t ropeDimensions,
                                             int mode)
{

    // contractsd ......
    if (!input || !input->isValid())
        throw std::invalid_argument{"RopeGraph requires a valid input tensor"};

    if (!positions.isValid())
        throw std::invalid_argument{"RopeGraph requires a valid position tensor"};

    if (ropeDimensions == 0)
        throw std::invalid_argument{"RopeGraph requires a rotary dimension greater than zero"};

    if (ropeDimensions > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument{"RopeGraph rotary dimension exceeds GGML integer range"};

    return input->rope(positions, static_cast<int>(ropeDimensions), mode);
}

ggml::JobGgmlTensorOp::UPtr RopeGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                             const ggml::JobGgmlTensor &positions,
                                             const RopeConfig *config,
                                             uint32_t fallbackDimensions,
                                             int mode)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{ "RopeGraph requires a valid input tensor" };

    if (!positions.isValid())
        throw std::invalid_argument{ "RopeGraph requires a valid position tensor" };

    const uint32_t ropeDimensions = config->ropeDimensionCount() > 0 ? config->ropeDimensionCount() : fallbackDimensions;

    if (ropeDimensions == 0)
        throw std::invalid_argument{ "RopeGraph requires a rotary dimension greater than zero" };

    if (ropeDimensions > static_cast<uint32_t>( std::numeric_limits<int>::max())) {
        throw std::invalid_argument{ "RopeGraph rotary dimension exceeds GGML integer range" };
    }


    // this is unacceptable. and should be from the RopeConfig That should be the only thing that is passed into this
    // function both fallbackDimensions and mode should be in the config. RopeExtGraph should be a thing....
    return input->ropeExt(positions,
                          nullptr,
                          static_cast<int>(ropeDimensions),
                          mode,
                          0,
                          config->ropeFreqBase(),
                          config->ropeFreqScale(),
                          0.0f,
                          1.0f,
                          0.0f,
                          0.0f);
}
} // namespace job::model