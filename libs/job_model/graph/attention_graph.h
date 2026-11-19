#pragma once

#include <cstdint>

#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>

#include "config/attention_config.h"
#include "jobmodel_export.h"
#include "kv/kv_cache.h"
#include "weights/model_weights.h"

namespace job::model {

class JOBMODEL_EXPORT AttentionGraph final
{
public:
    AttentionGraph() = delete;
    ~AttentionGraph() = delete;

    AttentionGraph(const AttentionGraph &) = delete;
    AttentionGraph &operator=(const AttentionGraph &) = delete;
    AttentionGraph(AttentionGraph &&) = delete;
    AttentionGraph &operator=(AttentionGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const LayerWeights &weights,
                                                           const AttentionConfig &config,
                                                           KvCache &kvCache,
                                                           const ggml::JobGgmlTensor &positions,
                                                           uint32_t layerIndex,
                                                           uint32_t nPast,
                                                           ggml::JobGgmlTensorOp::UPtr *kCacheWriteOut,
                                                           ggml::JobGgmlTensorOp::UPtr *vCacheWriteOut,
                                                           ggml::JobGgmlType inputType = ggml::JobGgmlType::F16,
                                                           const NormConfig *normConfig = nullptr,
                                                           const RopeConfig *ropeConfig = nullptr);
private:
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr tensorOp(const ggml::JobGgmlTensor &tensor,
                                                              ggml::JobGgmlContext *ctx);
};

} // namespace job::model
