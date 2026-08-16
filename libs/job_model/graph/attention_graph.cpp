#include "graph/attention_graph.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include <job_ggml_tensor_data.h>

#include "graph/gqa_graph.h"
#include "graph/linear_graph.h"
#include "graph/norm_graph.h"
#include "graph/rope_graph.h"

namespace job::model {

ggml::JobGgmlTensorOp::UPtr AttentionGraph::tensorOp(const ggml::JobGgmlTensor &tensor,
                                                     ggml::JobGgmlContext *ctx)
{
    return ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor *>(tensor.tensor()), ctx);
}

ggml::JobGgmlTensorOp::UPtr AttentionGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                                  const LayerWeights &weights,
                                                  const AttentionConfig &config,
                                                  KvCache &kvCache,
                                                  const ggml::JobGgmlTensor &positions,
                                                  uint32_t layerIndex,
                                                  uint32_t nPast,
                                                  float rmsNormEps,
                                                  uint32_t ropeDimensions,
                                                  int ropeMode,
                                                  ggml::JobGgmlType inputType)
{
    if (!input || !input->isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid input tensor"};

    if (!config.isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid AttentionConfig"};

    if (!positions.isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid position tensor"};

    if (positions.type() != ggml::JobGgmlType::I32)
        throw std::invalid_argument{"AttentionGraph position tensor must use I32"};

    if (!weights.attnQ || !weights.attnQ->isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid Q projection weight"};

    if (!weights.attnK || !weights.attnK->isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid K projection weight"};

    if (!weights.attnV || !weights.attnV->isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid V projection weight"};

    if (!weights.attnOut || !weights.attnOut->isValid())
        throw std::invalid_argument{"AttentionGraph requires a valid output projection weight"};

    if (layerIndex >= kvCache.layerCount())
        throw std::out_of_range{"AttentionGraph layer index exceeds KV cache layer count"};

    if (nPast > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument{"AttentionGraph nPast exceeds GGML integer range"};

    if (config.slidingWindowSize() > 0)
        throw std::invalid_argument{"AttentionGraph sliding-window attention is not implemented"};

    auto *ctx = input->context();

    if (!ctx || !ctx->isValid())
        throw std::invalid_argument{"AttentionGraph requires an input with a valid GGML context"};

    const uint32_t embeddingLength = static_cast<uint32_t>(input->extent(0));
    const uint32_t nTokens         = static_cast<uint32_t>(input->extent(1));

    if (embeddingLength == 0)
        throw std::invalid_argument{"AttentionGraph requires a positive embedding length"};

    if (nTokens == 0)
        throw std::invalid_argument{"AttentionGraph requires at least one input token"};

    if (positions.extent(0) != static_cast<int64_t>(nTokens))
        throw std::invalid_argument{"AttentionGraph position count must match token count"};

    const uint32_t headCount       = config.headCount();
    const uint32_t headCountKv     = config.headCountKv();
    const uint32_t headDimension   = config.headDimension(embeddingLength);
    const uint32_t headDimensionKv = config.headDimensionKv(embeddingLength);

    if (headCount == 0 || headCountKv == 0)
        throw std::invalid_argument{"AttentionGraph requires valid attention head counts"};

    if (headDimension == 0 || headDimensionKv == 0)
        throw std::invalid_argument{"AttentionGraph requires valid attention head dimensions"};

    const uint64_t totalContext = static_cast<uint64_t>(nPast) + static_cast<uint64_t>(nTokens);

    if (totalContext > kvCache.maxContextLength())
        throw std::out_of_range{"AttentionGraph exceeds KV cache context length"};

    //
    // Q / K / V projections.
    //
    auto q = LinearGraph::build(input->dup(),
                                *weights.attnQ,
                                weights.attnQBias.get(),
                                inputType);

    auto k = LinearGraph::build(input->dup(),
                                *weights.attnK,
                                weights.attnKBias.get(),
                                inputType);

    auto v = LinearGraph::build(std::move(input),
                                *weights.attnV,
                                weights.attnVBias.get(),
                                inputType);

    q = q->reshape3d(headDimension,
                     headCount,
                     nTokens);

    k = k->reshape3d(headDimensionKv,
                     headCountKv,
                     nTokens);

    v = v->reshape3d(headDimensionKv,
                     headCountKv,
                     nTokens);

    //
    // Optional Q / K normalization.
    //
    if (weights.attnQNorm) {
        auto qNormWeight =
            tensorOp(*weights.attnQNorm, ctx)->repeat(*q);

        q = NormGraph::rms(std::move(q),
                           *qNormWeight,
                           rmsNormEps);
    }

    if (weights.attnKNorm) {
        auto kNormWeight =
            tensorOp(*weights.attnKNorm, ctx)->repeat(*k);

        k = NormGraph::rms(std::move(k),
                           *kNormWeight,
                           rmsNormEps);
    }

    //
    // Rotary positional embedding.
    //
    q = RopeGraph::build(std::move(q),
                         positions,
                         ropeDimensions,
                         ropeMode);

    k = RopeGraph::build(std::move(k),
                         positions,
                         ropeDimensions,
                         ropeMode);

    //
    // Append this token range to the layer KV cache.
    //
    LayerKvEntry &kvEntry = kvCache.layer(layerIndex);

    if (!kvEntry.k || !kvEntry.k->isValid() || !kvEntry.v || !kvEntry.v->isValid()) {
        throw std::runtime_error{"AttentionGraph requires valid layer KV tensors"};
    }

    const int64_t kvRowSize = static_cast<int64_t>(headCountKv) * static_cast<int64_t>(headDimensionKv);

    auto kFlat = k->reshape2d(kvRowSize, nTokens);
    auto vFlat = v->reshape2d(kvRowSize, nTokens);

    ggml::JobGgmlTensorData kCacheData{kvEntry.k->tensor()};
    ggml::JobGgmlTensorData vCacheData{kvEntry.v->tensor()};

    const std::size_t kTypeSize = kCacheData.typeSize();
    const std::size_t vTypeSize = vCacheData.typeSize();
    const std::size_t kRowBytes = static_cast<std::size_t>(kvRowSize) * kTypeSize;

    const std::size_t vRowBytes = static_cast<std::size_t>(kvRowSize) * vTypeSize;

    const std::size_t kCacheOffset = static_cast<std::size_t>(nPast) * kRowBytes;

    const std::size_t vCacheOffset = static_cast<std::size_t>(nPast) * vRowBytes;

    auto kCacheView = tensorOp(*kvEntry.k, ctx)->view2d(kvRowSize,
                                                        nTokens,
                                                        kRowBytes,
                                                        kCacheOffset);

    auto vCacheView = tensorOp(*kvEntry.v, ctx)->view2d(kvRowSize,
                                                        nTokens,
                                                        vRowBytes,
                                                        vCacheOffset);

    (void)kFlat->cpy(*kCacheView);
    (void)vFlat->cpy(*vCacheView);

    //
    // Read the complete active cache range.
    //
    const int64_t totalCtx = static_cast<int64_t>(totalContext);

    auto kAll =
        tensorOp(*kvEntry.k, ctx)->view3d(
            headDimensionKv,
            headCountKv,
            totalCtx,
            static_cast<std::size_t>(headDimensionKv) * kTypeSize,
            kRowBytes,
            0);

    auto vAll =
        tensorOp(*kvEntry.v, ctx)->view3d(
            headDimensionKv,
            headCountKv,
            totalCtx,
            static_cast<std::size_t>(headDimensionKv) * vTypeSize,
            vRowBytes,
            0);

    //
    // MHA is a no-op here; GQA/MQA expand KV heads to query-head geometry.
    //
    kAll = GqaGraph::expand(std::move(kAll), headCount);
    vAll = GqaGraph::expand(std::move(vAll), headCount);

    //
    // Scaled dot-product attention.
    //
    const float scale = 1.0f / std::sqrt(static_cast<float>(headDimension));

    auto qTrans = q->permute(0, 2, 1, 3)->cont();
    auto kTrans = kAll->permute(0, 2, 1, 3)->cont();

    auto scores = kTrans->mulMat(*qTrans)->scale(scale);

    const float softCap = config.attnLogitSoftCapping();

    if (softCap > 0.0f) {
        scores = scores
                     ->scale(1.0f / softCap)
                     ->tanh()
                     ->scale(softCap);
    }

    auto probabilities =
        scores
            ->diagMaskInf(static_cast<int>(nPast))
            ->softMax();

    auto values =
        vAll
            ->permute(1, 2, 0, 3)
            ->cont();

    auto attentionHeads =
        values->mulMat(*probabilities);

    auto attention =
        attentionHeads
            ->permute(0, 2, 1, 3)
            ->cont()
            ->reshape2d(static_cast<int64_t>(headCount) * static_cast<int64_t>(headDimension), nTokens);

    //
    // Output projection.
    //
    return LinearGraph::build(std::move(attention),
                              *weights.attnOut,
                              weights.attnOutBias.get(),
                              inputType);
}

} // namespace job::model