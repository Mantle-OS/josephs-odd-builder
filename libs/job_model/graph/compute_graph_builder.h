#pragma once

#include <cstdint>
#include <memory>

#include <model_config.h>
#include <weights/model_weights.h>
#include <kv/kv_cache.h>

#include <job_ggml_context.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>
#include <job_ggml_cgraph.h>
#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>

namespace job::model {

class ComputeGraphBuilder {
public:
    ComputeGraphBuilder(const ModelConfig& config,
                        const ModelWeights& weights,
                        KvCache& kvCache,
                        ggml::JobGgmlBackend* backend,
                        ggml::JobGgmlBackendSched* scheduler);

    [[nodiscard]] ggml::JobGgmlCGraph::UPtr buildForwardGraph(
        ggml::JobGgmlContext& computeCtx,
        ggml::JobGgmlTensor& inputTokens,
        uint32_t nPast);

private:
    [[nodiscard]] ggml::JobGgmlTensorOp::UPtr buildLayerNorm(
        ggml::JobGgmlContext& ctx,
        ggml::JobGgmlTensorOp::UPtr x,
        const ggml::JobGgmlTensor* weight,
        const ggml::JobGgmlTensor* bias,
        float eps) const;

    [[nodiscard]] ggml::JobGgmlTensorOp::UPtr buildSelfAttention(
        ggml::JobGgmlContext& ctx,
        ggml::JobGgmlTensorOp::UPtr xNorm,
        const LayerWeights& lw,
        uint32_t layerIdx,
        uint32_t nTokens,
        uint32_t nPast);

    [[nodiscard]] ggml::JobGgmlTensorOp::UPtr buildFeedForward(
        ggml::JobGgmlContext& ctx,
        ggml::JobGgmlTensorOp::UPtr xNorm,
        const LayerWeights& lw,
        uint32_t layerIdx) const;

    const ModelConfig&          m_config;
    const ModelWeights&         m_weights;
    KvCache&                    m_kvCache;
    ggml::JobGgmlBackend*       m_backend;
    ggml::JobGgmlBackendSched*  m_scheduler;
};

} // namespace job::model