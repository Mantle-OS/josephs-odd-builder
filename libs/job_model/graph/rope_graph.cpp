#include "graph/rope_graph.h"

#include <limits>
#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr RopeGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                             const ggml::JobGgmlTensor &positions,
                                             uint32_t ropeDimensions,
                                             int mode)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"RopeGraph requires a valid input tensor"};

    if (!positions.isValid())
        throw std::invalid_argument{"RopeGraph requires a valid position tensor"};

    if (ropeDimensions == 0)
        throw std::invalid_argument{"RopeGraph requires a rotary dimension greater than zero"};

    if (ropeDimensions > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument{"RopeGraph rotary dimension exceeds GGML integer range"};

    return input->rope(positions,
                       static_cast<int>(ropeDimensions),
                       mode);
}

} // namespace job::model