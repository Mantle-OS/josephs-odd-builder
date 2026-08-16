#include "kv/kv_cache.h"

#include <format>
#include <job_logger.h>

#include <job_ggml_enums.h>

namespace job::model {

KvCache::~KvCache()
{
    clear();
}

KvCache::KvCache(KvCache&& other) noexcept
    : m_ctx(std::move(other.m_ctx))
    , m_layers(std::move(other.m_layers))
    , m_currentPos(other.m_currentPos)
    , m_maxCtx(other.m_maxCtx)
    , m_nHeadKv(other.m_nHeadKv)
    , m_headDimKv(other.m_headDimKv)
    , m_kvType(other.m_kvType)
    , m_totalSizeBytes(other.m_totalSizeBytes)
{
    other.m_currentPos = 0;
    other.m_maxCtx = 0;
    other.m_totalSizeBytes = 0;
}

KvCache& KvCache::operator=(KvCache&& other) noexcept
{
    if (this != &other) {
        clear();

        m_ctx = std::move(other.m_ctx);
        m_layers = std::move(other.m_layers);
        m_currentPos = other.m_currentPos;
        m_maxCtx = other.m_maxCtx;
        m_nHeadKv = other.m_nHeadKv;
        m_headDimKv = other.m_headDimKv;
        m_kvType = other.m_kvType;
        m_totalSizeBytes = other.m_totalSizeBytes;

        other.m_currentPos = 0;
        other.m_maxCtx = 0;
        other.m_totalSizeBytes = 0;
    }
    return *this;
}

void KvCache::clear() noexcept
{
    m_layers.clear();
    m_ctx.reset();
    m_currentPos = 0;
    m_maxCtx = 0;
    m_nHeadKv = 0;
    m_headDimKv = 0;
    m_totalSizeBytes = 0;
}

bool KvCache::init(const ModelConfig& config, uint32_t maxContextLength, ggml::JobGgmlType kvType)
{
    clear();

    const uint32_t numLayers = config.m_transformerConfig.m_blockCount;
    m_maxCtx    = (maxContextLength > 0) ? maxContextLength : config.m_transformerConfig.m_contextLength;
    m_nHeadKv   = config.m_transformerConfig.m_headCountKv;
    m_headDimKv = config.m_transformerConfig.headDimensionKv();
    m_kvType    = kvType;

    if (numLayers == 0 || m_maxCtx == 0 || m_nHeadKv == 0 || m_headDimKv == 0) {
        JOB_LOG_ERROR("[KvCache] Invalid parameters for KV cache init (Layers: {}, Ctx: {}, Heads: {}, Dim: {})",
                      numLayers, m_maxCtx, m_nHeadKv, m_headDimKv);
        return false;
    }

    const int64_t kvRowSize = static_cast<int64_t>(m_nHeadKv) * static_cast<int64_t>(m_headDimKv);

    // Calculate memory requirements: 2 tensors (K + V) * numLayers * [kvRowSize * maxCtx * typeSize]
    const enum ggml_type nativeGgmlType = ggml::toGgmlType(kvType);
    const size_t bytesPerElement = ggml_type_size(nativeGgmlType);
    const size_t tensorPayloadBytes = static_cast<size_t>(kvRowSize) * m_maxCtx * bytesPerElement;
    const size_t totalPayloadBytes = 2 * numLayers * tensorPayloadBytes;

    // Calculate overhead for tensor headers
    const size_t tensorOverhead = 2 * numLayers * ggml_tensor_overhead();
    m_totalSizeBytes = totalPayloadBytes + tensorOverhead + 1024; // Small padding margin

    ggml::JobGgmlInitParams initParams(m_totalSizeBytes, nullptr, false);
    m_ctx = ggml::JobGgmlContext::createUniq(initParams);

    if (!m_ctx || !m_ctx->isValid()) {
        JOB_LOG_ERROR("[KvCache] Failed to initialize JobGgmlContext for KV cache (Requested: {:.2f} MB)",
                      static_cast<double>(m_totalSizeBytes) / (1024.0 * 1024.0));
        clear();
        return false;
    }

    m_layers.resize(numLayers);

    for (uint32_t i = 0; i < numLayers; ++i) {
        LayerKvEntry& entry = m_layers[i];
        entry.layerIndex = i;

        // Allocate 2D tensors [kvRowSize, maxCtx]
        entry.k = m_ctx->newTensor2d(kvType, kvRowSize, m_maxCtx);
        entry.v = m_ctx->newTensor2d(kvType, kvRowSize, m_maxCtx);

        if (!entry.k || !entry.v || !entry.k->isValid() || !entry.v->isValid()) {
            JOB_LOG_ERROR("[KvCache] Failed to create KV tensors for layer {}", i);
            clear();
            return false;
        }

        entry.k->setName(std::format("cache_k_l{}", i));
        entry.v->setName(std::format("cache_v_l{}", i));
    }

    JOB_LOG_INFO("[KvCache] Allocated KV cache across {} layers (Ctx: {}, Heads: {}, Dim: {}, RAM: {:.2f} MB)",
                 numLayers, m_maxCtx, m_nHeadKv, m_headDimKv,
                 static_cast<double>(m_totalSizeBytes) / (1024.0 * 1024.0));

    return true;
}

const LayerKvEntry& KvCache::layer(uint32_t layerIdx) const
{
    if (layerIdx >= m_layers.size()) {
        static const LayerKvEntry s_empty{};
        JOB_LOG_ERROR("[KvCache] Layer index {} out of range (total: {})", layerIdx, m_layers.size());
        return s_empty;
    }
    return m_layers[layerIdx];
}

LayerKvEntry& KvCache::layer(uint32_t layerIdx)
{
    if (layerIdx >= m_layers.size()) {
        static LayerKvEntry s_empty{};
        JOB_LOG_ERROR("[KvCache] Layer index {} out of range (total: {})", layerIdx, m_layers.size());
        return s_empty;
    }
    return m_layers[layerIdx];
}

} // namespace job::model