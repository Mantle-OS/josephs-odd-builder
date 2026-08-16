#include "kv/kv_cache.h"

#include <format>
#include <job_logger.h>

#include <job_ggml_enums.h>
#include <job_ggml_device.h>
#include <alloc/job_ggml_tensor_allocator.h>

namespace job::model {

KvCache::~KvCache()
{
    clear();
}

KvCache::KvCache(KvCache&& other) noexcept
    : m_ctx(std::move(other.m_ctx))
    , m_buffer(std::move(other.m_buffer))
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
        m_buffer = std::move(other.m_buffer);
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
    m_buffer.reset(); // Release buffer memory first before destroying context descriptors
    m_ctx.reset();
    m_currentPos = 0;
    m_maxCtx = 0;
    m_nHeadKv = 0;
    m_headDimKv = 0;
    m_totalSizeBytes = 0;
}

bool KvCache::init(const ModelConfig& config, ggml::JobGgmlBackend* backend, uint32_t maxContextLength, ggml::JobGgmlType kvType)
{
    clear();

    if (!backend || !backend->isValid()) {
        JOB_LOG_ERROR("[KvCache] Cannot initialize KV cache: borrowed backend is null or invalid");
        return false;
    }

    const uint32_t numLayers = config.transformerConfig().blockCount();
    m_maxCtx    = (maxContextLength > 0) ? maxContextLength : config.transformerConfig().contextLength();
    m_nHeadKv   = config.attentionConfig().headCountKv();
    m_headDimKv = config.attentionConfig().headDimensionKv();
    m_kvType    = kvType;

    if (numLayers == 0 || m_maxCtx == 0 || m_nHeadKv == 0 || m_headDimKv == 0) {
        JOB_LOG_ERROR("[KvCache] Invalid parameters for KV cache init (Layers: {}, Ctx: {}, Heads: {}, Dim: {})",
                      numLayers, m_maxCtx, m_nHeadKv, m_headDimKv);
        return false;
    }

    const int64_t kvRowSize = static_cast<int64_t>(m_nHeadKv) * static_cast<int64_t>(m_headDimKv);

    // 1. Create Metadata-Only Context (no_alloc = true)
    // Estimate size strictly for tensor descriptor overheads, not payload bytes
    const size_t tensorOverhead = 2 * numLayers * ggml_tensor_overhead();
    const size_t metaCtxSize = tensorOverhead + 4096;

    ggml::JobGgmlInitParams initParams(metaCtxSize, nullptr, true);
    m_ctx = ggml::JobGgmlContext::createUniq(initParams);

    if (!m_ctx || !m_ctx->isValid()) {
        JOB_LOG_ERROR("[KvCache] Failed to initialize metadata JobGgmlContext for KV cache");
        clear();
        return false;
    }

    m_layers.resize(numLayers);

    // 2. Instantiate 2D tensor descriptors across all layers
    for (uint32_t i = 0; i < numLayers; ++i) {
        LayerKvEntry& entry = m_layers[i];
        entry.layerIndex = i;

        entry.k = m_ctx->newTensor2d(kvType, kvRowSize, m_maxCtx);
        entry.v = m_ctx->newTensor2d(kvType, kvRowSize, m_maxCtx);

        if (!entry.k || !entry.v || !entry.k->isValid() || !entry.v->isValid()) {
            JOB_LOG_ERROR("[KvCache] Failed to create KV tensor descriptors for layer {}", i);
            clear();
            return false;
        }

        entry.k->setName(std::format("cache_k_l{}", i));
        entry.v->setName(std::format("cache_v_l{}", i));
    }

    // 3. Use JobGgmlTensorAllocator to compute buffer size and allocate physical VRAM buffer via backend device buffer type
    auto* device = backend->device();
    if (!device || !device->isValid()) {
        JOB_LOG_ERROR("[KvCache] Failed to retrieve valid device from borrowed backend");
        clear();
        return false;
    }

    auto buft = device->bufferType();
    if (!buft) {
        JOB_LOG_ERROR("[KvCache] Failed to retrieve backend buffer type from device");
        clear();
        return false;
    }

    // Initialize allocator for the context graph/tensors
    ggml::JobGgmlTensorAllocator allocator(buft);

    // Add all KV tensors to the allocator allocation list
    for (uint32_t i = 0; i < numLayers; ++i) {
        allocator.addTensor(*m_layers[i].k);
        allocator.addTensor(*m_layers[i].v);
    }

    m_totalSizeBytes = allocator.size();
    m_buffer = allocator.alloc();

    if (!m_buffer || !m_buffer->isValid()) {
        JOB_LOG_ERROR("[KvCache] Failed to allocate physical backend buffer for KV cache ({:.2f} MB)",
                      static_cast<double>(m_totalSizeBytes) / (1024.0 * 1024.0));
        clear();
        return false;
    }

    JOB_LOG_INFO("[KvCache] Natively allocated VRAM KV cache across {} layers (Ctx: {}, Heads: {}, Dim: {}, VRAM: {:.2f} MB)",
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