#pragma once

#include <cstdint>
#include <memory>

#include <job_ggml_tensor_shapes.h>

#include "jobmodel_export.h"

namespace job::model {

// Attention-block execution knobs and GQA/MHA/MQA geometry.
// Pulled out of ArchConfig/TransformerConfig because these are exactly
// the parameters the attention graph-builder piece consumes directly --
// not architecture identity, not generic transformer shape. The
// projection-shape helpers take embeddingLength as an explicit parameter
// rather than reaching into TransformerConfig: it's a real cross-config
// dependency, so the caller (whoever already holds both configs) supplies
// it instead of this class assuming which TransformerConfig it belongs to.
class JOBMODEL_EXPORT AttentionConfig
{
public:
    using Ptr  = std::shared_ptr<AttentionConfig>;
    using WPtr = std::weak_ptr<AttentionConfig>;
    using UPtr = std::unique_ptr<AttentionConfig>;

    AttentionConfig();
    ~AttentionConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<AttentionConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<AttentionConfig>(); }

    AttentionConfig(const AttentionConfig &) = default;
    AttentionConfig &operator=(const AttentionConfig &) = default;
    AttentionConfig(AttentionConfig &&) noexcept = default;
    AttentionConfig &operator=(AttentionConfig &&) noexcept = default;

    // Whether Q/K/V/O projections carry bias tensors (Qwen / Phi / StarCoder style).
    [[nodiscard]] bool attentionBias() const noexcept { return m_attentionBias; }
    void setAttentionBias(bool value) noexcept { m_attentionBias = value; }

    // Soft-capping applied to attention logits before softmax (Gemma 2 style).
    // 0.0f means disabled. Throws if value is non-finite or negative.
    [[nodiscard]] float attnLogitSoftCapping() const noexcept { return m_attnLogitSoftCapping; }
    void setAttnLogitSoftCapping(float value);

    // Local/windowed attention span. 0 means full (unwindowed) attention.
    [[nodiscard]] uint32_t slidingWindowSize() const noexcept { return m_slidingWindowSize; }
    void setSlidingWindowSize(uint32_t size) noexcept { m_slidingWindowSize = size; }

    // Attention Geometry (MHA / GQA / MQA support)
    [[nodiscard]] uint32_t headCount() const noexcept { return m_headCount; }
    void setHeadCount(uint32_t value) noexcept { m_headCount = value; }

    [[nodiscard]] uint32_t headCountKv() const noexcept { return m_headCountKv; }
    void setHeadCountKv(uint32_t value) noexcept { m_headCountKv = value; }

    [[nodiscard]] uint32_t keyLength() const noexcept { return m_keyLength; }     // head_dim
    void setKeyLength(uint32_t value) noexcept { m_keyLength = value; }

    [[nodiscard]] uint32_t valueLength() const noexcept { return m_valueLength; } // head_dim_kv
    void setValueLength(uint32_t value) noexcept { m_valueLength = value; }

    // headDimension()/headDimensionKv() only know keyLength/valueLength --
    // they return 0 if those aren't explicitly set (some GGUF/HF configs
    // omit them and expect embeddingLength / headCount instead). Use the
    // embeddingLength-taking overloads below when you need that fallback;
    // the *Shape() helpers already do.
    [[nodiscard]] uint32_t headDimension() const noexcept;
    [[nodiscard]] uint32_t headDimensionKv() const noexcept;
    [[nodiscard]] uint32_t headDimension(uint32_t embeddingLength) const noexcept;
    [[nodiscard]] uint32_t headDimensionKv(uint32_t embeddingLength) const noexcept;

    // Semantic tensor shape helpers for downstream graph builders.
    // embeddingLength is TransformerConfig::embeddingLength() -- passed in
    // explicitly rather than looked up, since this class doesn't hold a
    // TransformerConfig reference.
    [[nodiscard]] ggml::JobGgmlLinearShape qProjectionShape(uint32_t embeddingLength) const noexcept;
    [[nodiscard]] ggml::JobGgmlLinearShape kProjectionShape(uint32_t embeddingLength) const noexcept;
    [[nodiscard]] ggml::JobGgmlLinearShape vProjectionShape(uint32_t embeddingLength) const noexcept;
    [[nodiscard]] ggml::JobGgmlLinearShape outProjectionShape(uint32_t embeddingLength) const noexcept;
    [[nodiscard]] ggml::JobGgmlBSHDShape qActivationShape(int64_t batch, int64_t seq) const noexcept;
    [[nodiscard]] ggml::JobGgmlBSHDShape kvActivationShape(int64_t batch, int64_t seq) const noexcept;

    [[nodiscard]] bool isValid() const noexcept;

private:
    bool     m_attentionBias{false};
    float    m_attnLogitSoftCapping{0.0f};
    uint32_t m_slidingWindowSize{0};

    uint32_t m_headCount{0};
    uint32_t m_headCountKv{0};
    uint32_t m_keyLength{0};
    uint32_t m_valueLength{0};
};

} // namespace job::model