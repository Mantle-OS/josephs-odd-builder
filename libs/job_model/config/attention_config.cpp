#include "attention_config.h"

#include <stdexcept>

#include <real_type.h>

namespace job::model {

AttentionConfig::AttentionConfig() = default;

void AttentionConfig::setAttnLogitSoftCapping(float value)
{
    if (!core::isSafeFinite(value) || value < 0.0f) {
        throw std::invalid_argument{
            "AttentionConfig attnLogitSoftCapping must be finite and non-negative"
        };
    }

    m_attnLogitSoftCapping = value;
}

uint32_t AttentionConfig::headDimension() const noexcept
{
    // No embeddingLength in scope here -- 0 means "unknown, use the
    // embeddingLength-taking overload instead".
    return m_keyLength;
}

uint32_t AttentionConfig::headDimensionKv() const noexcept
{
    if (m_valueLength > 0)
        return m_valueLength;
    return headDimension();
}

uint32_t AttentionConfig::headDimension(uint32_t embeddingLength) const noexcept
{
    if (m_keyLength > 0)
        return m_keyLength;
    if (m_headCount > 0)
        return embeddingLength / m_headCount;
    return 0;
}

uint32_t AttentionConfig::headDimensionKv(uint32_t embeddingLength) const noexcept
{
    if (m_valueLength > 0)
        return m_valueLength;
    return headDimension(embeddingLength);
}

ggml::JobGgmlLinearShape AttentionConfig::qProjectionShape(uint32_t embeddingLength) const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(m_headCount * headDimension(embeddingLength)),
        .inputDimension  = static_cast<int64_t>(embeddingLength)
    };
}

ggml::JobGgmlLinearShape AttentionConfig::kProjectionShape(uint32_t embeddingLength) const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(m_headCountKv * headDimensionKv(embeddingLength)),
        .inputDimension  = static_cast<int64_t>(embeddingLength)
    };
}

ggml::JobGgmlLinearShape AttentionConfig::vProjectionShape(uint32_t embeddingLength) const noexcept
{
    return kProjectionShape(embeddingLength);
}

ggml::JobGgmlLinearShape AttentionConfig::outProjectionShape(uint32_t embeddingLength) const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(embeddingLength),
        .inputDimension  = static_cast<int64_t>(m_headCount * headDimension(embeddingLength))
    };
}

ggml::JobGgmlBSHDShape AttentionConfig::qActivationShape(int64_t batch, int64_t seq) const noexcept
{
    return ggml::JobGgmlBSHDShape{
        .batch         = batch,
        .sequence      = seq,
        .heads         = static_cast<int64_t>(m_headCount),
        .headDimension = static_cast<int64_t>(headDimension())
    };
}

ggml::JobGgmlBSHDShape AttentionConfig::kvActivationShape(int64_t batch, int64_t seq) const noexcept
{
    return ggml::JobGgmlBSHDShape{
        .batch         = batch,
        .sequence      = seq,
        .heads         = static_cast<int64_t>(m_headCountKv),
        .headDimension = static_cast<int64_t>(headDimensionKv())
    };
}

bool AttentionConfig::isValid() const noexcept
{
    // slidingWindowSize and attentionBias have no invalid representation
    // on their own -- slidingWindowSize's real constraint (must not exceed
    // TransformerConfig::contextLength()) is a cross-config check this
    // class can't see, so it stays out here, same as embeddingLength above.
    return core::isSafeFinite(m_attnLogitSoftCapping) &&
           m_attnLogitSoftCapping >= 0.0f &&
           m_headCount > 0;
}

} // namespace job::model