#include "graph/compute_graph_builder.h"
#include "job_ggml_backend.h"
#include "job_ggml_backend_sched.h"

#include <cmath>
#include <format>
#include <job_logger.h>

#include <job_ggml_enums.h>
#include <job_ggml_tensor_data.h>
#include <job_ggml_tensor_op_graph.h>

namespace job::model {

ComputeGraphBuilder::ComputeGraphBuilder(
    const ModelConfig& config,
    const ModelWeights& weights,
    KvCache& kvCache,
    ggml::JobGgmlBackend* backend,
    ggml::JobGgmlBackendSched* scheduler)
    : m_config(config)
    , m_weights(weights)
    , m_kvCache(kvCache)
    , m_backend(backend)
    , m_scheduler(scheduler)
{
}

ggml::JobGgmlTensorOp::UPtr ComputeGraphBuilder::buildLayerNorm(
    ggml::JobGgmlContext& ctx,
    ggml::JobGgmlTensorOp::UPtr x,
    const ggml::JobGgmlTensor* weight,
    const ggml::JobGgmlTensor* bias,
    float eps) const
{
    if (!weight) return x;

    auto normed = x->rmsNorm(eps);
    auto scaled = normed->mul(*weight);

    if (bias && bias->isValid()) {
        scaled = scaled->add(*bias);
    }

    return scaled;
}

ggml::JobGgmlTensorOp::UPtr ComputeGraphBuilder::buildSelfAttention(
    ggml::JobGgmlContext& ctx,
    ggml::JobGgmlTensorOp::UPtr xNorm,
    const LayerWeights& lw,
    uint32_t layerIdx,
    uint32_t nTokens,
    uint32_t nPast)
{
    const uint32_t nHead      = m_config.m_transformerConfig.m_headCount;
    const uint32_t nHeadKv    = m_config.m_transformerConfig.m_headCountKv;
    const uint32_t dHead      = m_config.m_transformerConfig.headDimension();
    const uint32_t dHeadKv    = m_config.m_transformerConfig.headDimensionKv();
    const uint32_t nRot       = m_config.m_transformerConfig.m_ropeDimensionCount;
    const float    rmsEps     = m_config.m_transformerConfig.m_rmsNormEps;

    // 1. Q, K, V Linear Projections (Weight wrapped in JobGgmlTensorOp, activation cast to F16)
    JOB_LOG_INFO("[Graph] Layer {}: attnQ mulMat", layerIdx);
    auto q = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.attnQ->tensor()), &ctx)
                 ->mulMat(*xNorm->cast(ggml::JobGgmlType::F16));
    JOB_LOG_INFO("[Graph] Layer {}: attnK mulMat", layerIdx);
    auto k = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.attnK->tensor()), &ctx)
                 ->mulMat(*xNorm->cast(ggml::JobGgmlType::F16));
    JOB_LOG_INFO("[Graph] Layer {}: attnV mulMat", layerIdx);
    auto v = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.attnV->tensor()), &ctx)
                 ->mulMat(*xNorm->cast(ggml::JobGgmlType::F16));

    if (lw.attnQBias) q = q->add(*lw.attnQBias);
    if (lw.attnKBias) k = k->add(*lw.attnKBias);
    if (lw.attnVBias) v = v->add(*lw.attnVBias);

    // 3. Reshape Q, K, V to 3D [dHead, nHead, nTokens] FIRST
    q = q->reshape3d(dHead, nHead, nTokens);
    k = k->reshape3d(dHeadKv, nHeadKv, nTokens);
    v = v->reshape3d(dHeadKv, nHeadKv, nTokens);

    // 2. Q/K Normalization applied in 3D space
    if (lw.attnQNorm) {
        auto qNormWeight = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.attnQNorm->tensor()), &ctx)->repeat(*q);
        q = q->rmsNorm(rmsEps)->mul(*qNormWeight);
    }
    if (lw.attnKNorm) {
        auto kNormWeight = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.attnKNorm->tensor()), &ctx)->repeat(*k);
        k = k->rmsNorm(rmsEps)->mul(*kNormWeight);
    }

    // 4. Create position tensor and apply RoPE
    auto posTensor = ctx.newTensor1d(ggml::JobGgmlType::I32, nTokens);
    ggml::JobGgmlTensorData posData(posTensor->tensor());
    for (uint32_t j = 0; j < nTokens; ++j) {
        posData.setValueI32(j, static_cast<int32_t>(nPast + j));
    }

    // CRITICAL: Explicitly register posTensor with backend so scheduler does not evaluate it as -1
    if (m_backend && m_scheduler) {
        m_scheduler->setTensorBackend(*posTensor, *m_backend);
    }

    q = q->rope(*posTensor, static_cast<int>(nRot), 2);
    k = k->rope(*posTensor, static_cast<int>(nRot), 2);

    // 5. Store K, V into KV Cache Ring Buffer
    auto& kvEntry = m_kvCache.layer(layerIdx);
    const int64_t kvRowSize = static_cast<int64_t>(nHeadKv * dHeadKv);

    auto kFlat = k->reshape2d(kvRowSize, nTokens);
    auto vFlat = v->reshape2d(kvRowSize, nTokens);

    const size_t typeSize = ggml_type_size(ggml::toGgmlType(m_kvCache.type()));
    const size_t cacheOffset = static_cast<size_t>(nPast) * static_cast<size_t>(kvRowSize) * typeSize;

    auto kCacheView = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(kvEntry.k->tensor()), &ctx)->view2d(
        kvRowSize, nTokens, kvRowSize * typeSize, cacheOffset);
    auto vCacheView = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(kvEntry.v->tensor()), &ctx)->view2d(
        kvRowSize, nTokens, kvRowSize * typeSize, cacheOffset);

    (void)kFlat->cpy(*kCacheView);
    (void)vFlat->cpy(*vCacheView);

    // 6. Attention Score Computation
    const uint32_t totalCtx = nPast + nTokens;
    auto kAll = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(kvEntry.k->tensor()), &ctx)->view3d(
        dHeadKv, nHeadKv, totalCtx, dHeadKv * typeSize, kvRowSize * typeSize, 0);
    auto vAll = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(kvEntry.v->tensor()), &ctx)->view3d(
        dHeadKv, nHeadKv, totalCtx, dHeadKv * typeSize, kvRowSize * typeSize, 0);

    if (nHeadKv < nHead) {
        const int64_t nRep = static_cast<int64_t>(nHead / nHeadKv);
        kAll = kAll->reshape4d(dHeadKv, nHeadKv, 1, totalCtx)
                   ->repeat4d(dHeadKv, nHeadKv, nRep, totalCtx)
                   ->reshape3d(dHeadKv, nHead, totalCtx)
                   ->cont();
        vAll = vAll->reshape4d(dHeadKv, nHeadKv, 1, totalCtx)
                   ->repeat4d(dHeadKv, nHeadKv, nRep, totalCtx)
                   ->reshape3d(dHeadKv, nHead, totalCtx)
                   ->cont();
    }

    const float scale = 1.0f / std::sqrt(static_cast<float>(dHead));

    auto qTrans = q->permute(0, 2, 1, 3)->cont();
    auto kTrans = kAll->permute(0, 2, 1, 3)->cont();

    JOB_LOG_INFO("[Graph] Layer {}: qk score mulMat", layerIdx);
    auto qk = kTrans->mulMat(*qTrans);
    auto qkScaled = qk->scale(scale);

    if (m_config.m_archConfig.m_attnLogitSoftCapping > 0.0f) {
        const float cap = m_config.m_archConfig.m_attnLogitSoftCapping;
        qkScaled = qkScaled->scale(1.0f / cap)->tanh()->scale(cap);
    }

    auto qkMasked = qkScaled->diagMaskInf(static_cast<int>(nPast));
    auto qkSoftmax = qkMasked->softMax();

    auto vPerm = vAll->permute(1, 2, 0, 3)->cont();
    JOB_LOG_INFO("[Graph] Layer {}: value projection mulMat", layerIdx);
    auto attnOutHeads = vPerm->mulMat(*qkSoftmax);

    auto attnOut = attnOutHeads->permute(0, 2, 1, 3)->cont()->reshape2d(nHead * dHead, nTokens);

    // 7. Output Projection O (Weight wrapped in JobGgmlTensorOp, activation cast to F16)
    JOB_LOG_INFO("[Graph] Layer {}: attnOut projection mulMat", layerIdx);
    auto attnOutOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.attnOut->tensor()), &ctx);
    auto result = attnOutOp->mulMat(*attnOut->cast(ggml::JobGgmlType::F16));
    if (lw.attnOutBias) result = result->add(*lw.attnOutBias);

    return result;
}

ggml::JobGgmlTensorOp::UPtr ComputeGraphBuilder::buildFeedForward(
    ggml::JobGgmlContext& ctx,
    ggml::JobGgmlTensorOp::UPtr xNorm,
    const LayerWeights& lw,
    uint32_t layerIdx) const
{
    auto xCont = xNorm->cont();

    JOB_LOG_INFO("[Graph] Layer {}: ffnGate mulMat", layerIdx);
    auto gateOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.ffnGate->tensor()), &ctx);
    auto gate = gateOp->mulMat(*xCont->cast(ggml::JobGgmlType::F16));
    if (lw.ffnGateBias)
        gate = gate->add(*lw.ffnGateBias);
    auto gateAct = gate->silu();

    JOB_LOG_INFO("[Graph] Layer {}: ffnUp mulMat", layerIdx);
    auto upOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.ffnUp->tensor()), &ctx);
    auto up = upOp->mulMat(*xCont->cast(ggml::JobGgmlType::F16));
    if (lw.ffnUpBias) up = up->add(*lw.ffnUpBias);

    auto hidden = gateAct->mul(*up);

    const int64_t intermediateSize = lw.ffnDown->extent(0);
    const int64_t nTokens = hidden->extent(1);

    auto hiddenTrans = hidden->reshape2d(intermediateSize, nTokens)->cont();

    JOB_LOG_INFO("[Graph] Layer {}: ffnDown mulMat: hiddenTrans ne=[{}, {}] | ffnDown ne=[{}, {}]",
                 layerIdx,
                 hiddenTrans->extent(0), hiddenTrans->extent(1),
                 lw.ffnDown->extent(0), lw.ffnDown->extent(1));

    auto downOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(lw.ffnDown->tensor()), &ctx);
    auto out = downOp->mulMat(*hiddenTrans->cast(ggml::JobGgmlType::F16));
    if (lw.ffnDownBias) out = out->add(*lw.ffnDownBias);

    return out;
}

ggml::JobGgmlCGraph::UPtr ComputeGraphBuilder::buildForwardGraph(
    ggml::JobGgmlContext& computeCtx,
    ggml::JobGgmlTensor& inputTokens,
    uint32_t nPast)
{
    if (!m_weights.isLoaded() || !m_kvCache.isAllocated()) {
        JOB_LOG_ERROR("[ComputeGraphBuilder] Weights or KV cache not initialized");
        return nullptr;
    }

    const uint32_t nTokens   = static_cast<uint32_t>(inputTokens.extent(0));
    const uint32_t numLayers = m_config.m_transformerConfig.m_blockCount;
    const float    rmsEps    = m_config.m_transformerConfig.m_rmsNormEps;

    // 1. Ingest Token Embeddings
    auto embdOp = ggml::JobGgmlTensorOp::createUniq(const_cast<struct ggml_tensor*>(m_weights.tokenEmbd()->tensor()), &computeCtx);
    auto cur = embdOp->getRows(inputTokens);

    const uint32_t embedLen = m_config.m_transformerConfig.m_embeddingLength;

    // 2. Iterate Transformer Layers
    for (uint32_t i = 0; i < numLayers; ++i) {
        const auto& lw = m_weights.layer(i);

        // Pre-Attention Norm
        auto attnNorm = buildLayerNorm(computeCtx, cur->dup(), lw.attnNorm.get(), lw.attnNormBias.get(), rmsEps);

        // Self-Attention & Residual
        auto attnOut = buildSelfAttention(computeCtx, std::move(attnNorm), lw, i, nTokens, nPast);
        if (lw.postAttnNorm) {
            attnOut = buildLayerNorm(computeCtx, std::move(attnOut), lw.postAttnNorm.get(), nullptr, rmsEps);
        }

        auto attnCont = attnOut->reshape2d(embedLen, nTokens)->cont();
        cur = cur->add(*attnCont);

        // Pre-FFN Norm
        auto ffnNorm = buildLayerNorm(computeCtx, cur->dup(), lw.ffnNorm.get(), lw.ffnNormBias.get(), rmsEps);

        // Feed-Forward & Residual
        auto ffnOut = buildFeedForward(computeCtx, std::move(ffnNorm), lw, i);
        if (lw.postFfnNorm) {
            ffnOut = buildLayerNorm(computeCtx, std::move(ffnOut), lw.postFfnNorm.get(), nullptr, rmsEps);
        }

        auto ffnCont = ffnOut->reshape2d(embedLen, nTokens)->cont();
        cur = cur->add(*ffnCont);
    }

    // 3. Final Output Norm
    cur = buildLayerNorm(computeCtx, std::move(cur), m_weights.outputNorm(), m_weights.outputNormBias(), rmsEps);

    // 4. Final LM Head Projection
    auto outputHead = m_weights.output();
    if (!outputHead || !outputHead->isValid()) {
        outputHead = m_weights.tokenEmbd();
    }

    auto outputOp = ggml::JobGgmlTensorOp::createUniq(
        const_cast<struct ggml_tensor*>(outputHead->tensor()), &computeCtx);

    JOB_LOG_INFO("[Graph] Final LM Head projection mulMat");
    auto logits = outputOp->mulMat(*cur->cast(ggml::JobGgmlType::F16));

    if (m_config.m_archConfig.m_finalLogitSoftCapping > 0.0f) {
        const float cap = m_config.m_archConfig.m_finalLogitSoftCapping;
        logits = logits->scale(1.0f / cap)->tanh()->scale(cap);
    }

    logits->setName("logits");

    // 5. Expand & Build Computation Graph
    auto graphOp = ggml::JobGgmlTensorOpGraph::wrap(std::move(logits));
    return graphOp->buildGraph();
}

} // namespace job::model