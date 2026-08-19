#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "config/model_config.h"
#include "graph/gated_ffn_graph.h"
#include "graph/graph_builder.h"
#include "kv/kv_cache.h"
#include "weights/model_weights.h"

#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT QwenGraphBuilder final : public GraphBuilder
{
public:
    using Ptr  = std::shared_ptr<QwenGraphBuilder>;
    using WPtr = std::weak_ptr<QwenGraphBuilder>;
    using UPtr = std::unique_ptr<QwenGraphBuilder>;

    QwenGraphBuilder(const ModelConfig &config,
                     const ModelWeights &weights,
                     KvCache &kvCache);

    ~QwenGraphBuilder() override = default;

    [[nodiscard]] static Ptr createShared(const ModelConfig &config,
                                          const ModelWeights &weights,
                                          KvCache &kvCache)
    {
        return std::make_shared<QwenGraphBuilder>(config, weights, kvCache);
    }

    [[nodiscard]] static UPtr createUniq(const ModelConfig &config,
                                         const ModelWeights &weights,
                                         KvCache &kvCache)
    {
        return std::make_unique<QwenGraphBuilder>(config, weights, kvCache);
    }

    QwenGraphBuilder(const QwenGraphBuilder &) = delete;
    QwenGraphBuilder &operator=(const QwenGraphBuilder &) = delete;
    QwenGraphBuilder(QwenGraphBuilder &&) noexcept = default;
    QwenGraphBuilder &operator=(QwenGraphBuilder &&) noexcept = delete;

    [[nodiscard]] ggml::JobGgmlCGraph::UPtr buildForward(ggml::JobGgmlContext &ctx,
                                                         ggml::JobGgmlTensor &inputTokens,
                                                         uint32_t nPast,
                                                         ggml::JobGgmlType inputType = ggml::JobGgmlType::F16) override;

private:
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr expandGqa(ggml::JobGgmlTensorOp::UPtr input, uint32_t queryHeadCount);
    [[nodiscard]] static GatedFfnGraph::Activation activation(std::string_view name);

    const ModelConfig  &m_config;
    const ModelWeights &m_weights;
    KvCache            &m_kvCache;
};

} // namespace job::model