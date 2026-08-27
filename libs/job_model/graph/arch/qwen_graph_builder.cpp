#include "graph/arch/qwen_graph_builder.h"

#include <stdexcept>
#include <vector>

// #include <job_ggml_tensor_op_graph.h>

#include "graph/attention_graph.h"
#include "graph/embedding_graph.h"
#include "graph/linear_graph.h"
#include "graph/norm_graph.h"
#include "graph/residual_graph.h"

namespace job::model {

QwenGraphBuilder::QwenGraphBuilder(const ModelConfig &config,
                                   const ModelWeights &weights,
                                   KvCache &kvCache) :
    m_config{config},
    m_weights{weights},
    m_kvCache{kvCache}
{
    if (!m_config.isValid())
        throw std::invalid_argument{"QwenGraphBuilder requires a valid ModelConfig"};

    if (!m_weights.isLoaded())
        throw std::invalid_argument{"QwenGraphBuilder requires loaded model weights"};
}

ggml::JobGgmlCGraph::UPtr QwenGraphBuilder::buildForward(ggml::JobGgmlContext &ctx,
                                                         ggml::JobGgmlTensor &inputTokens,
                                                         uint32_t nPast,
                                                         ggml::JobGgmlType inputType)
{
    if (!ctx.isValid())
        throw std::invalid_argument{"QwenGraphBuilder requires a valid GGML context"};

    if (!inputTokens.isValid())
        throw std::invalid_argument{"QwenGraphBuilder requires a valid input token tensor"};

    if (!m_weights.tokenEmbd())
        throw std::runtime_error{"QwenGraphBuilder requires token embedding weights"};

    if (!m_weights.outputNorm())
        throw std::runtime_error{"QwenGraphBuilder requires output normalization weights"};

    const TransformerConfig     &transformerConfig = m_config.transformerConfig();
    const AttentionConfig       &attentionConfig   = m_config.attentionConfig();
    const NormConfig            &normConfig        = m_config.normConfig();
    const RopeConfig            &ropeConfig        = m_config.ropeConfig();
    const OutputHeadConfig      &outputConfig      = m_config.outputHeadConfig();
    const uint32_t nTokens = static_cast<uint32_t>(inputTokens.extent(0));


    // these could all be contracts
    if (nTokens == 0)
        throw std::invalid_argument{"QwenGraphBuilder requires at least one input token"};

    if (m_weights.layerCount() != transformerConfig.blockCount())
        throw std::runtime_error{"QwenGraphBuilder weight layer count does not match model configuration"};

    if (m_kvCache.layerCount() != transformerConfig.blockCount())
        throw std::runtime_error{"QwenGraphBuilder KV layer count does not match model configuration"};

    const uint32_t headDimension = attentionConfig.headDimension(transformerConfig.embeddingLength());
    if (headDimension == 0)
        throw std::runtime_error{"QwenGraphBuilder could not resolve attention head dimension"};

    const uint32_t ropeDimensions = ropeConfig.ropeDimensionCount() > 0 ? ropeConfig.ropeDimensionCount() : headDimension;

    std::vector<ggml::JobGgmlTensorOp::UPtr> cacheWrites;
    cacheWrites.reserve(static_cast<std::size_t>(transformerConfig.blockCount()) * 2);

    // Token embedding.
    auto cur = EmbeddingGraph::build(ctx, inputTokens, *m_weights.tokenEmbd());

    //
    // Positions are graph-produced data rather than host-filled tensors.
    // This keeps position storage under normal graph allocation/scheduling
    // instead of recreating the old setTensorBackend() placement hack.
    //
    auto positions = cur->arange(static_cast<float>(nPast),
                                static_cast<float>(nPast + nTokens), 1.0f)->cast(ggml::JobGgmlType::I32);

    const GatedFfnGraph::Activation hiddenActivation = activation(m_config.archConfig().hiddenActivation());

    // Qwen transformer blocks:
    //
    //   x = x + Attention(RMSNorm(x))
    //   x = x + FFN(RMSNorm(x))
    for (uint32_t layerIndex = 0; layerIndex < transformerConfig.blockCount(); ++layerIndex) {

        const LayerWeights &weights = m_weights.layer(layerIndex);
        if (!weights.attnNorm || !weights.attnNorm->isValid())
            throw std::runtime_error{"QwenGraphBuilder requires attention normalization weights"};

        if (!weights.ffnNorm || !weights.ffnNorm->isValid())
            throw std::runtime_error{"QwenGraphBuilder requires feed-forward normalization weights"};

        auto attentionInput = NormGraph::rms(cur->dup(),
                                             *weights.attnNorm,
                                             normConfig.rmsNormEps(),
                                             weights.attnNormBias.get());

        ggml::JobGgmlTensorOp::UPtr kCacheWrite;
        ggml::JobGgmlTensorOp::UPtr vCacheWrite;

        auto attention = AttentionGraph::build(std::move(attentionInput),
                                               weights,
                                               attentionConfig,
                                               m_kvCache,
                                               *positions,
                                               layerIndex,
                                               nPast,
                                               normConfig.rmsNormEps(),
                                               ropeDimensions,
                                               &kCacheWrite,
                                               &vCacheWrite,
                                               2,
                                               inputType,
                                               &ropeConfig);


        cacheWrites.push_back(std::move(kCacheWrite));
        cacheWrites.push_back(std::move(vCacheWrite));

        cur = ResidualGraph::build(std::move(cur),
                                   std::move(attention));

        auto ffnInput = NormGraph::rms(cur->dup(),
                           *weights.ffnNorm,
                           normConfig.rmsNormEps(),
                           weights.ffnNormBias.get());


        auto ffn = GatedFfnGraph::build(std::move(ffnInput),
                                 weights,
                                 hiddenActivation,
                                 inputType);

        cur = ResidualGraph::build(std::move(cur),
                                   std::move(ffn));
    }


    // Final RMSNorm.
    cur = NormGraph::rms(std::move(cur),
                         *m_weights.outputNorm(),
                         normConfig.rmsNormEps(),
                         m_weights.outputNormBias());


    // LM head. ModelWeights::output() already handles tied embeddings by
    // returning tokenEmbd() when there is no dedicated output tensor.
    const ggml::JobGgmlTensor *outputWeight = m_weights.output();

    if (!outputWeight || !outputWeight->isValid())
        throw std::runtime_error{"QwenGraphBuilder requires valid output weights"};

    auto logits = LinearGraph::build(std::move(cur),
                                     *outputWeight,
                                     nullptr,
                                     inputType);

    const float softCap = outputConfig.finalLogitSoftCapping();

    if (softCap > 0.0f)
        logits = logits->scale(1.0f / softCap)->tanh()->scale(softCap);

    logits->setName("logits");

    //  FIXME 8192 lives where ?
    auto graph = ctx.newGraphCustom(8192, false);

    if (!graph)
        throw std::runtime_error{"QwenGraphBuilder failed to create forward graph"};

    for (auto &cacheWrite : cacheWrites) {
        if (!cacheWrite || !cacheWrite->isValid())
            throw std::runtime_error{"QwenGraphBuilder received an invalid KV cache write"};

        graph->buildForwardExpand(*cacheWrite);
    }

    graph->buildForwardExpand(*logits);

    return graph;
}

GatedFfnGraph::Activation QwenGraphBuilder::activation(std::string_view name)
{
    if (name == "silu" || name == "swiglu")
        return GatedFfnGraph::Activation::Silu;

    if (name == "gelu")
        return GatedFfnGraph::Activation::Gelu;

    if (name == "relu")
        return GatedFfnGraph::Activation::Relu;

    throw std::invalid_argument{"QwenGraphBuilder received an unsupported hidden activation"};
}

} // namespace job::model