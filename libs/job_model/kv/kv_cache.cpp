#include "kv/kv_cache.h"

#include <format>
#include <stdexcept>

#include <alloc/job_ggml_tensor_allocator.h>
#include <job_ggml_init_params.h>

#ifndef NDEBUG
#include <job_logger.h>
#endif
namespace job::model {

KvCache::KvCache(const ModelConfig &config, const ggml::JobGgmlBackendBufferType &bufferType, uint32_t maxContextLength, ggml::JobGgmlType kvType) :
    m_maxCtx{maxContextLength > 0 ? maxContextLength : config.transformerConfig().contextLength()},
    m_nHeadKv{config.attentionConfig().headCountKv()},
    m_headDimKv{config.attentionConfig().headDimensionKv(config.transformerConfig().embeddingLength())},
    m_kvType{kvType}
{
    const uint32_t numLayers = config.transformerConfig().blockCount();

    if (!bufferType.isValid())
        throw std::invalid_argument{"KvCache requires a valid JobGgmlBackendBufferType"};

    if (numLayers == 0)
        throw std::invalid_argument{"KvCache requires at least one transformer layer"};

    if (m_maxCtx == 0)
        throw std::invalid_argument{"KvCache requires a context length greater than zero"};

    if (m_nHeadKv == 0)
        throw std::invalid_argument{"KvCache requires at least one KV head"};

    if (m_headDimKv == 0)
        throw std::invalid_argument{"KvCache requires a KV head dimension greater than zero"};

    const std::size_t tensorCount = static_cast<std::size_t>(numLayers) * 2;

    // The context owns tensor metadata only. K/V payload storage is owned by
    // m_buffer and allocated using the caller-resolved backend buffer type.
    m_ctx = ggml::JobGgmlContext::createUniqMetadata(tensorCount);
    if (!m_ctx || !m_ctx->isValid())
        throw std::runtime_error{"Failed to create KV cache metadata context"};

    const int64_t kvRowSize =
        static_cast<int64_t>(m_nHeadKv) *
        static_cast<int64_t>(m_headDimKv);

    m_layers.reserve(numLayers);

    for (uint32_t i = 0; i < numLayers; ++i) {
        LayerKvEntry entry;
        entry.layerIndex = i;

        entry.k = m_ctx->newTensor2d(m_kvType, kvRowSize, static_cast<int64_t>(m_maxCtx));
        entry.v = m_ctx->newTensor2d(m_kvType, kvRowSize, static_cast<int64_t>(m_maxCtx));

        if (!entry.k || !entry.k->isValid())
            throw std::runtime_error{ std::format("Failed to create KV key tensor for layer {}", i) };

        if (!entry.v || !entry.v->isValid())
            throw std::runtime_error{ std::format("Failed to create KV value tensor for layer {}", i) };

        entry.k->setName(std::format("cache_k_l{}", i));
        entry.v->setName(std::format("cache_v_l{}", i));

        m_layers.emplace_back(std::move(entry));
    }

    // Determine the physical buffer size using the actual buffer type selected
    // by the runtime. Backend buffer types may impose alignment and allocation
    // requirements that differ from the tensor's logical byte size.
    const std::size_t alignment = bufferType.alignment();
    std::size_t requiredBytes = 0;
    for (const LayerKvEntry &entry : m_layers) {
        requiredBytes = alignUp(requiredBytes, alignment);
        requiredBytes += bufferType.allocationSize(*entry.k);

        requiredBytes = alignUp(requiredBytes, alignment);
        requiredBytes += bufferType.allocationSize(*entry.v);
    }

    if (requiredBytes == 0)
        throw std::runtime_error{"KV cache resolved to an empty backend allocation"};

    // JobGgmlBackendBufferType currently returns unique ownership. Promote the
    // allocated buffer into the shared ownership model used by
    // JobGgmlTensorAllocator.
    m_buffer = ggml::JobGgmlBackendBuffer::Ptr{
        bufferType.allocateBuffer(requiredBytes)
    };

    if (!m_buffer || !m_buffer->isValid())
        throw std::runtime_error{"Failed to allocate KV cache backend buffer"};

    ggml::JobGgmlTensorAllocator allocator{m_buffer};

    if (!allocator.isValid())
        throw std::runtime_error{"Failed to create KV cache tensor allocator"};

    for (LayerKvEntry &entry : m_layers) {
        if (allocator.allocate(*entry.k) != ggml::JobGgmlStatus::Success) {
            throw std::runtime_error{
                std::format("Failed to allocate KV key tensor for layer {}", entry.layerIndex)
            };
        }

        if (allocator.allocate(*entry.v) != ggml::JobGgmlStatus::Success) {
            throw std::runtime_error{
                std::format("Failed to allocate KV value tensor for layer {}", entry.layerIndex)
            };
        }
    }

    m_totalSizeBytes = m_buffer->size();

#ifndef NDEBUG
    JOB_LOG_INFO(
        "[KvCache] Allocated KV cache: layers={}, ctx={}, heads={}, dim={}, buffer={}, size={:.2f} MB",
        numLayers,
        m_maxCtx,
        m_nHeadKv,
        m_headDimKv,
        bufferType.name(),
        static_cast<double>(m_totalSizeBytes) / (1024.0 * 1024.0));
#endif

}

const LayerKvEntry &KvCache::layer(uint32_t layerIdx) const
{
    return m_layers.at(layerIdx);
}

LayerKvEntry &KvCache::layer(uint32_t layerIdx)
{
    return m_layers.at(layerIdx);
}

} // namespace job::model