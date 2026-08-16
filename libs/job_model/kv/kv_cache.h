#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <job_ggml_backend_buffer.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>

#include "config/model_config.h"
#include "jobmodel_export.h"

namespace job::model {

struct JOBMODEL_EXPORT LayerKvEntry
{
    uint32_t layerIndex{0};

    // [head_dim_kv * n_head_kv, n_ctx]
    ggml::JobGgmlTensor::UPtr k;

    // [head_dim_kv * n_head_kv, n_ctx]
    ggml::JobGgmlTensor::UPtr v;
};

class JOBMODEL_EXPORT KvCache
{
public:
    using Ptr  = std::shared_ptr<KvCache>;
    using WPtr = std::weak_ptr<KvCache>;
    using UPtr = std::unique_ptr<KvCache>;

    explicit KvCache(const ModelConfig &config,
                     const ggml::JobGgmlBackendBufferType &bufferType,
                     uint32_t maxContextLength = 0,
                     ggml::JobGgmlType kvType = ggml::JobGgmlType::F16);

    ~KvCache() = default;

    KvCache(const KvCache &) = delete;
    KvCache &operator=(const KvCache &) = delete;

    KvCache(KvCache &&) noexcept = default;
    KvCache &operator=(KvCache &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(const ModelConfig &config,
                                          const ggml::JobGgmlBackendBufferType &bufferType,
                                          uint32_t maxContextLength = 0,
                                          ggml::JobGgmlType kvType = ggml::JobGgmlType::F16)
    {
        return std::make_shared<KvCache>(config, bufferType, maxContextLength, kvType);
    }

    [[nodiscard]] static UPtr createUniq(const ModelConfig &config,
                                         const ggml::JobGgmlBackendBufferType &bufferType,
                                         uint32_t maxContextLength = 0,
                                         ggml::JobGgmlType kvType = ggml::JobGgmlType::F16)
    {
        return std::make_unique<KvCache>(config, bufferType, maxContextLength, kvType);
    }

    void resetPosition() noexcept { m_currentPos = 0; }

    [[nodiscard]] uint32_t currentPosition() const noexcept { return m_currentPos; }
    void advance(uint32_t count = 1) noexcept { m_currentPos += count; }
    void setPosition(uint32_t pos) noexcept { m_currentPos = pos; }

    [[nodiscard]] uint32_t maxContextLength() const noexcept { return m_maxCtx; }
    [[nodiscard]] uint32_t layerCount() const noexcept { return static_cast<uint32_t>(m_layers.size()); }
    [[nodiscard]] uint32_t headCountKv() const noexcept { return m_nHeadKv; }
    [[nodiscard]] uint32_t headDimensionKv() const noexcept { return m_headDimKv; }
    [[nodiscard]] ggml::JobGgmlType type() const noexcept { return m_kvType; }
    [[nodiscard]] std::size_t totalSizeBytes() const noexcept { return m_totalSizeBytes; }

    [[nodiscard]] const LayerKvEntry &layer(uint32_t layerIdx) const;
    [[nodiscard]] LayerKvEntry &layer(uint32_t layerIdx);

    [[nodiscard]] ggml::JobGgmlContext *context() noexcept { return m_ctx.get(); }
    [[nodiscard]] const ggml::JobGgmlContext *context() const noexcept { return m_ctx.get(); }

    [[nodiscard]] ggml::JobGgmlBackendBuffer *buffer() noexcept { return m_buffer.get(); }
    [[nodiscard]] const ggml::JobGgmlBackendBuffer *buffer() const noexcept { return m_buffer.get(); }

private:
    ggml::JobGgmlContext::UPtr      m_ctx;
    ggml::JobGgmlBackendBuffer::Ptr m_buffer;

    std::vector<LayerKvEntry> m_layers;

    uint32_t m_currentPos{0};
    uint32_t m_maxCtx{0};
    uint32_t m_nHeadKv{0};
    uint32_t m_headDimKv{0};

    ggml::JobGgmlType m_kvType{ggml::JobGgmlType::F16};

    std::size_t m_totalSizeBytes{0};

    [[nodiscard]] static constexpr std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept
    {
        if (alignment <= 1)
            return value;

        const std::size_t remainder = value % alignment;
        return remainder == 0 ? value : value + alignment - remainder;
    }
};

} // namespace job::model