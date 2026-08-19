#include "graph/gqa_graph.h"

#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr GqaGraph::expand(ggml::JobGgmlTensorOp::UPtr input,
                                             uint32_t queryHeadCount)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"GqaGraph requires a valid input tensor"};

    if (!input->isThreeDimensional())
        throw std::invalid_argument{"GqaGraph requires a three-dimensional input tensor"};

    if (queryHeadCount == 0)
        throw std::invalid_argument{"GqaGraph requires at least one query head"};

    const int64_t headDimension = input->extent(0);
    const int64_t kvHeadCount   = input->extent(1);
    const int64_t contextLength = input->extent(2);

    if (headDimension <= 0)
        throw std::invalid_argument{"GqaGraph requires a positive head dimension"};

    if (kvHeadCount <= 0)
        throw std::invalid_argument{"GqaGraph requires at least one KV head"};

    if (contextLength <= 0)
        throw std::invalid_argument{"GqaGraph requires a positive context length"};

    if (static_cast<int64_t>(queryHeadCount) < kvHeadCount)
        throw std::invalid_argument{"GqaGraph query head count cannot be less than KV head count"};

    if (static_cast<int64_t>(queryHeadCount) == kvHeadCount)
        return input;

    if (static_cast<int64_t>(queryHeadCount) % kvHeadCount != 0)
        throw std::invalid_argument{"GqaGraph query head count must be divisible by KV head count"};


    const int64_t repeatCount = static_cast<int64_t>(queryHeadCount) / kvHeadCount;

    return input
        ->reshape4d(headDimension,
                    kvHeadCount,
                    1,
                    contextLength)
        ->repeat4d(headDimension,
                   kvHeadCount,
                   repeatCount,
                   contextLength)
        ->reshape3d(headDimension,
                    static_cast<int64_t>(queryHeadCount),
                    contextLength)
        ->cont();
}

} // namespace job::model