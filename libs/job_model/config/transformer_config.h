#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <job_ggml_tensor_shapes.h>

#include "jobmodel_export.h"

namespace job::model {

// Core, genuinely global transformer dimensions -- values needed across
// every block (embeddings, attention, FFN, output head), not exclusive
// to any one graph-builder piece. Attention geometry (headCount/GQA),
// normalization epsilon, RoPE, and FFN width have all been split out into
// AttentionConfig / NormConfig / RopeConfig / FeedForwardConfig -- each
// consumed by exactly one piece, unlike the fields that remain here.
class JOBMODEL_EXPORT TransformerConfig
{
public:
    using Ptr  = std::shared_ptr<TransformerConfig>;
    using WPtr = std::weak_ptr<TransformerConfig>;
    using UPtr = std::unique_ptr<TransformerConfig>;

    TransformerConfig();
    ~TransformerConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<TransformerConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<TransformerConfig>(); }

    TransformerConfig(const TransformerConfig &) = default;
    TransformerConfig &operator=(const TransformerConfig &) = default;
    TransformerConfig(TransformerConfig &&) noexcept = default;
    TransformerConfig &operator=(TransformerConfig &&) noexcept = default;

    // Maximum sequence length the model supports (drives KV cache sizing).
    [[nodiscard]] uint32_t contextLength() const noexcept { return m_contextLength; }
    void setContextLength(uint32_t value) noexcept { m_contextLength = value; }

    // Residual stream / hidden dimension. Touched by every block.
    [[nodiscard]] uint32_t embeddingLength() const noexcept { return m_embeddingLength; }
    void setEmbeddingLength(uint32_t value) noexcept { m_embeddingLength = value; }

    // Number of transformer layers.
    [[nodiscard]] uint32_t blockCount() const noexcept { return m_blockCount; }
    void setBlockCount(uint32_t value) noexcept { m_blockCount = value; }

    // Vocabulary size. Needed by both the token embedding table (input)
    // and the LM head (output) -- not exclusive to one block.
    [[nodiscard]] uint32_t vocabSize() const noexcept { return m_vocabSize; }
    void setVocabSize(uint32_t value) noexcept { m_vocabSize = value; }

    [[nodiscard]] bool isValid() const noexcept;

    // Semantic tensor shape helper for downstream graph builders.
    [[nodiscard]] ggml::JobGgmlVDShape tokenEmbeddingShape() const noexcept;

private:
    uint32_t m_contextLength{0};
    uint32_t m_embeddingLength{0};
    uint32_t m_blockCount{0};
    uint32_t m_vocabSize{0};
};

} // namespace job::model