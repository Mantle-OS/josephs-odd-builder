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

//  we already a have a config for this why in the heck is thisinherit of the config itsself

class JOBMODEL_EXPORT AttentionGraph final
{
public:
    AttentionGraph() = delete;
    ~AttentionGraph() = delete;

    AttentionGraph(const AttentionGraph &) = delete;
    AttentionGraph &operator=(const AttentionGraph &) = delete;
    AttentionGraph(AttentionGraph &&) = delete;
    AttentionGraph &operator=(AttentionGraph &&) = delete;


    // This is doing way way way to much.
    // This needs to be broken up into opher graphs
// ggml::JobGgmlTensorOp::UPtr input,  PREFECT
// const LayerWeights &weights, PREFECT
// const AttentionConfig &config,   IF THIS IS ALREADY THIS (Inhertierd)
// KvCache &kvCache,   PREFECT
// const ggml::JobGgmlTensor &positions,  FINE for now
// uint32_t layerIndex,         # BAD should be in the config Or some other config not a options
// uint32_t nPast,              # BAD should be in the config Or some other config not a options
// float rmsNormEps,            # BAD  should be in the config Or some other config not a options
// uint32_t ropeDimensions,     # BAD should be in the config Or some other config not a options
// ggml::JobGgmlTensorOp::UPtr *kCacheWriteOut,  Good
// ggml::JobGgmlTensorOp::UPtr *vCacheWriteOut,  GOOD
// int ropeMode = 2,                              BAD
// ggml::JobGgmlType inputType = ggml::JobGgmlType::F16,  HACKY But fine for now
// const RopeConfig *ropeConfig = nullptr); PREFECT


    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const LayerWeights &weights,
                                                           const AttentionConfig &config,
                                                           KvCache &kvCache,
                                                           const ggml::JobGgmlTensor &positions,
                                                           uint32_t layerIndex,
                                                           uint32_t nPast,
                                                           float rmsNormEps,
                                                           uint32_t ropeDimensions,
                                                           ggml::JobGgmlTensorOp::UPtr *kCacheWriteOut,
                                                           ggml::JobGgmlTensorOp::UPtr *vCacheWriteOut,
                                                           int ropeMode = 2,
                                                           ggml::JobGgmlType inputType = ggml::JobGgmlType::F16,
                                                           const RopeConfig *ropeConfig = nullptr);
private:
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr tensorOp(const ggml::JobGgmlTensor &tensor,
                                                              ggml::JobGgmlContext *ctx);
};

} // namespace job::model