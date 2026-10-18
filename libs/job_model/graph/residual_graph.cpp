#include "graph/residual_graph.h"

#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr ResidualGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                                 ggml::JobGgmlTensorOp::UPtr residual)
{
#ifndef NDEBUG
    if (!input || !input->isValid())
        throw std::invalid_argument{"ResidualGraph requires a valid input tensor"};

    if (!residual || !residual->isValid())
        throw std::invalid_argument{"ResidualGraph requires a valid residual tensor"};
#endif

    return input->add(*residual);
}

} // namespace job::model