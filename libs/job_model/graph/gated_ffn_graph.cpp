#include "graph/gated_ffn_graph.h"

#include <stdexcept>

#include "graph/linear_graph.h"

namespace job::model {

ggml::JobGgmlTensorOp::UPtr GatedFfnGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                                 const LayerWeights &weights,
                                                 Activation activation,
                                                 ggml::JobGgmlType inputType)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"GatedFfnGraph requires a valid input tensor"};

    if (!weights.ffnGate || !weights.ffnGate->isValid())
        throw std::invalid_argument{"GatedFfnGraph requires a valid gate projection weight"};

    if (!weights.ffnUp || !weights.ffnUp->isValid())
        throw std::invalid_argument{"GatedFfnGraph requires a valid up projection weight"};

    if (!weights.ffnDown || !weights.ffnDown->isValid())
        throw std::invalid_argument{"GatedFfnGraph requires a valid down projection weight"};

    if (weights.ffnGateBias && !weights.ffnGateBias->isValid())
        throw std::invalid_argument{"GatedFfnGraph gate bias is invalid"};

    if (weights.ffnUpBias && !weights.ffnUpBias->isValid())
        throw std::invalid_argument{"GatedFfnGraph up bias is invalid"};

    if (weights.ffnDownBias && !weights.ffnDownBias->isValid())
        throw std::invalid_argument{"GatedFfnGraph down bias is invalid"};

    auto contiguousInput = input->cont();

    auto gate = LinearGraph::build(contiguousInput->dup(),
                                   *weights.ffnGate,
                                   weights.ffnGateBias.get(),
                                   inputType);

    gate = activate(std::move(gate), activation);

    auto up = LinearGraph::build(std::move(contiguousInput),
                                 *weights.ffnUp,
                                 weights.ffnUpBias.get(),
                                 inputType);

    auto hidden = gate->mul(*up);

    const int64_t intermediateSize = weights.ffnDown->extent(0);
    const int64_t nTokens          = hidden->extent(1);

    if (intermediateSize <= 0)
        throw std::invalid_argument{"GatedFfnGraph requires a positive intermediate dimension"};

    if (nTokens <= 0)
        throw std::invalid_argument{"GatedFfnGraph requires at least one token"};

    auto hiddenContiguous =
        hidden
            ->reshape2d(intermediateSize, nTokens)
            ->cont();

    return LinearGraph::build(std::move(hiddenContiguous),
                              *weights.ffnDown,
                              weights.ffnDownBias.get(),
                              inputType);
}

ggml::JobGgmlTensorOp::UPtr GatedFfnGraph::activate(ggml::JobGgmlTensorOp::UPtr input,
                                                    Activation activation)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"GatedFfnGraph activation requires a valid input tensor"};

    switch (activation) {
    case Activation::Silu:
        return input->silu();

    case Activation::Gelu:
        return input->gelu();

    case Activation::Relu:
        return input->relu();
    }

    throw std::invalid_argument{"GatedFfnGraph received an unsupported activation"};
}

} // namespace job::model