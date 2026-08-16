#include "graph/norm_graph.h"

#include <cmath>
#include <stdexcept>
#include <real_type.h>
namespace job::model {

ggml::JobGgmlTensorOp::UPtr NormGraph::rms(ggml::JobGgmlTensorOp::UPtr input,
                                           const ggml::JobGgmlTensor &weight,
                                           float eps,
                                           const ggml::JobGgmlTensor *bias)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"NormGraph requires a valid input tensor"};

    if (!weight.isValid())
        throw std::invalid_argument{"NormGraph requires a valid weight tensor"};

    if (bias && !bias->isValid())
        throw std::invalid_argument{"NormGraph requires a valid bias tensor when provided"};

    if (!core::isSafeFinite(eps) || eps <= 0.0f)
        throw std::invalid_argument{"NormGraph requires epsilon to be finite and greater than zero"};

    auto result = input->rmsNorm(eps)->mul(weight);

    if (bias)
        result = result->add(*bias);

    return result;
}

} // namespace job::model