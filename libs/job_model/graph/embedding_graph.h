#pragma once

#include <job_ggml_context.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"

namespace job::model {

// we already(or should) a have a config for this why in the heck is this inherit of the config itsself
class JOBMODEL_EXPORT EmbeddingGraph final
{
public:
    EmbeddingGraph() = delete;
    ~EmbeddingGraph() = delete;

    EmbeddingGraph(const EmbeddingGraph &) = delete;
    EmbeddingGraph &operator=(const EmbeddingGraph &) = delete;
    EmbeddingGraph(EmbeddingGraph &&) = delete;
    EmbeddingGraph &operator=(EmbeddingGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlContext &ctx,
                                                           const ggml::JobGgmlTensor &tokens,
                                                           const ggml::JobGgmlTensor &weight);
};

} // namespace job::model