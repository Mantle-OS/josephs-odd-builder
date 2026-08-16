#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <job_ggml_backend.h>
#include <job_ggml_backend_buffer.h>

#include "config/model_config.h"
#include "jobmodel_export.h"

namespace job::model {

struct JOBMODEL_EXPORT LayerKvEntry {
    uint32_t layerIndex{0};
    ggml::JobGgmlTensor::UPtr k; // Key tensor:   [head_dim_kv * n_head_kv, n_ctx]
    ggml::JobGgmlTensor::UPtr v; // Value tensor: [head_dim_kv * n_head_kv, n_ctx]
};

class JOBMODEL_EXPORT KvCache {
public:
    using Ptr  = std::shared_ptr<KvCache>;
    using UPtr = std::unique_ptr<KvCache>;

    KvCache() = default;
    ~KvCache();

    KvCache(const KvCache&) = delete;
    KvCache& operator=(const KvCache&) = delete;
    KvCache(KvCache&&) noexcept;
    KvCache& operator=(KvCache&&) noexcept;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<KvCache>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<KvCache>();
    }

    // Allocate KV storage tensors natively inside a borrowed backend buffer using Option A
    [[nodiscard]] bool init(
        const ModelConfig& config,
        ggml::JobGgmlBackend* backend,
        uint32_t maxContextLength = 0,
        ggml::JobGgmlType kvType = ggml::JobGgmlType::F16);

    void clear() noexcept;
    void resetPosition() noexcept { m_currentPos = 0; }

    // Head position tracking
    [[nodiscard]] uint32_t currentPosition() const noexcept { return m_currentPos; }
    void advance(uint32_t count = 1) noexcept { m_currentPos += count; }
    void setPosition(uint32_t pos) noexcept { m_currentPos = pos; }

    // Dimensions & State
    [[nodiscard]] uint32_t maxContextLength() const noexcept { return m_maxCtx; }
    [[nodiscard]] uint32_t layerCount() const noexcept { return static_cast<uint32_t>(m_layers.size()); }
    [[nodiscard]] uint32_t headCountKv() const noexcept { return m_nHeadKv; }
    [[nodiscard]] uint32_t headDimensionKv() const noexcept { return m_headDimKv; }
    [[nodiscard]] ggml::JobGgmlType type() const noexcept { return m_kvType; }
    [[nodiscard]] size_t totalSizeBytes() const noexcept { return m_totalSizeBytes; }
    [[nodiscard]] bool isAllocated() const noexcept {
        return !m_layers.empty() && m_ctx != nullptr && m_buffer != nullptr && m_buffer->isValid();
    }

    // Per-layer tensor accessors
    [[nodiscard]] const LayerKvEntry& layer(uint32_t layerIdx) const;
    [[nodiscard]] LayerKvEntry& layer(uint32_t layerIdx);

    // Owning Context & Buffer
    [[nodiscard]] ggml::JobGgmlContext* context() noexcept { return m_ctx.get(); }
    [[nodiscard]] const ggml::JobGgmlContext* context() const noexcept { return m_ctx.get(); }
    [[nodiscard]] ggml::JobGgmlBackendBuffer* buffer() noexcept { return m_buffer.get(); }

private:
    ggml::JobGgmlContext::UPtr       m_ctx;
    ggml::JobGgmlBackendBuffer::UPtr m_buffer; // Explicit owner of the device VRAM buffer
    std::vector<LayerKvEntry>        m_layers;

    uint32_t          m_currentPos{0};
    uint32_t          m_maxCtx{0};
    uint32_t          m_nHeadKv{0};
    uint32_t          m_headDimKv{0};
    ggml::JobGgmlType m_kvType{ggml::JobGgmlType::F16};
    size_t            m_totalSizeBytes{0};
};

} // namespace job::model