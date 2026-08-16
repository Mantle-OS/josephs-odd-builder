#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <job_ggml_tensor_shapes.h>

// #include "arch_config.h"
#include "jobmodel_export.h"

namespace job::model {

// The core blueprint for a transformer network.
// Zero vtables. Pure value semantics. Ready to be composed or inherited by specific model families.
struct JOBMODEL_EXPORT TransformerConfig {
    // Core Transformer Dimensions
    uint32_t          m_contextLength{4096};
    uint32_t          m_embeddingLength{4096};
    uint32_t          m_blockCount{32};
    uint32_t          m_feedForwardLength{11008};
    uint32_t          m_vocabSize{32000};

    // Attention Geometry (MHA / GQA / MQA support)
    uint32_t          m_headCount{32};
    uint32_t          m_headCountKv{32};
    uint32_t          m_keyLength{0};     // head_dim
    uint32_t          m_valueLength{0};   // head_dim_kv

    // Normalization Epsilon
    float             m_rmsNormEps{1e-5f};
    float             m_layerNormEps{1e-5f};

    // RoPE (Rotary Positional Embeddings) parameters
    uint32_t          m_ropeDimensionCount{0};
    float             m_ropeFreqBase{10000.0f};
    float             m_ropeFreqScale{1.0f};

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // Don't build a transformer with zero layers or missing dimensions,
        // unless you enjoy spending your evenings chasing segmentation faults.
        return
               m_blockCount > 0 &&
               m_embeddingLength > 0 &&
               m_headCount > 0 &&
               m_vocabSize > 0;
    }


    [[nodiscard]] uint32_t headDimension() const noexcept
    {
        if (m_keyLength > 0)
            return m_keyLength;
        if (m_headCount > 0)
            return m_embeddingLength / m_headCount;
        return 0;
    }

    [[nodiscard]] uint32_t headDimensionKv() const noexcept
    {
        if (m_valueLength > 0)
            return m_valueLength;
        return headDimension();
    }

    // Semantic tensor shape helpers for downstream graph builders
    [[nodiscard]] ggml::JobGgmlVDShape tokenEmbeddingShape() const noexcept
    {
        return ggml::JobGgmlVDShape{
            .vocabulary = static_cast<int64_t>(m_vocabSize),
            .dimension  = static_cast<int64_t>(m_embeddingLength)
        };
    }

    [[nodiscard]] ggml::JobGgmlLinearShape qProjectionShape() const noexcept
    {
        return ggml::JobGgmlLinearShape{
            .outputDimension = static_cast<int64_t>(m_headCount * headDimension()),
            .inputDimension  = static_cast<int64_t>(m_embeddingLength)
        };
    }

    [[nodiscard]] ggml::JobGgmlLinearShape kProjectionShape() const noexcept
    {
        return ggml::JobGgmlLinearShape{
            .outputDimension = static_cast<int64_t>(m_headCountKv * headDimensionKv()),
            .inputDimension  = static_cast<int64_t>(m_embeddingLength)
        };
    }

    [[nodiscard]] ggml::JobGgmlLinearShape vProjectionShape() const noexcept
    {
        return ggml::JobGgmlLinearShape{
            .outputDimension = static_cast<int64_t>(m_headCountKv * headDimensionKv()),
            .inputDimension  = static_cast<int64_t>(m_embeddingLength)
        };
    }

    [[nodiscard]] ggml::JobGgmlLinearShape outProjectionShape() const noexcept
    {
        return ggml::JobGgmlLinearShape{
            .outputDimension = static_cast<int64_t>(m_embeddingLength),
            .inputDimension  = static_cast<int64_t>(m_headCount * headDimension())
        };
    }

    [[nodiscard]] ggml::JobGgmlBSHDShape qActivationShape(int64_t batch, int64_t seq) const noexcept
    {
        return ggml::JobGgmlBSHDShape{
            .batch         = batch,
            .sequence      = seq,
            .heads         = static_cast<int64_t>(m_headCount),
            .headDimension = static_cast<int64_t>(headDimension())
        };
    }

    [[nodiscard]] ggml::JobGgmlBSHDShape kvActivationShape(int64_t batch, int64_t seq) const noexcept
    {
        return ggml::JobGgmlBSHDShape{
            .batch         = batch,
            .sequence      = seq,
            .heads         = static_cast<int64_t>(m_headCountKv),
            .headDimension = static_cast<int64_t>(headDimensionKv())
        };
    }
};

} // namespace job::model