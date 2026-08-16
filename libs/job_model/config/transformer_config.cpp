#include "transformer_config.h"

namespace job::model {

TransformerConfig::TransformerConfig() = default;

bool TransformerConfig::isValid() const noexcept
{
    return m_contextLength > 0 &&
           m_embeddingLength > 0 &&
           m_blockCount > 0 &&
           m_vocabSize > 0;
}

ggml::JobGgmlVDShape TransformerConfig::tokenEmbeddingShape() const noexcept
{
    return ggml::JobGgmlVDShape{
        .vocabulary = static_cast<int64_t>(m_vocabSize),
        .dimension  = static_cast<int64_t>(m_embeddingLength)
    };
}

} // namespace job::model