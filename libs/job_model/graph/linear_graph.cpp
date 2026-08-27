#include "graph/linear_graph.h"

#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr LinearGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                               const ggml::JobGgmlTensor &weight,
                                               const ggml::JobGgmlTensor *bias,
                                               ggml::JobGgmlType inputType)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"LinearGraph requires a valid input tensor"};

    if (!weight.isValid())
        throw std::invalid_argument{"LinearGraph requires a valid weight tensor"};

    if (bias && !bias->isValid())
        throw std::invalid_argument{"LinearGraph requires a valid bias tensor when provided"};

    auto *ctx = input->context();

    if (!ctx || !ctx->isValid())
        throw std::invalid_argument{"LinearGraph requires an input with a valid GGML context"};

    auto weightOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor *>(weight.tensor()),
                                                      ctx);

    auto result = weightOp->mulMat(*input->cast(inputType));
    if (bias)
        result = result->add(*bias);

    return result;
}

} // namespace job::model