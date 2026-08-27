#include "graph/embedding_graph.h"

#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr EmbeddingGraph::build(ggml::JobGgmlContext &ctx,
                                                  const ggml::JobGgmlTensor &tokens,
                                                  const ggml::JobGgmlTensor &weight)
{
    if (!ctx.isValid())
        throw std::invalid_argument{"EmbeddingGraph requires a valid GGML context"};

    if (!tokens.isValid())
        throw std::invalid_argument{"EmbeddingGraph requires a valid token tensor"};

    if (!weight.isValid())
        throw std::invalid_argument{"EmbeddingGraph requires a valid embedding weight tensor"};

    if (tokens.type() != ggml::JobGgmlType::I32)
        throw std::invalid_argument{"EmbeddingGraph token tensor must use I32"};

    auto weightOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor *>(weight.tensor()), &ctx);
    return weightOp->getRows(tokens);
}

} // namespace job::model