#pragma once

#include "model_config.h"

namespace job::model::arch::qwen {

// The official blueprint for Qwen3-4B-Instruct-2507.
// 256k context window, 36 layers, and a massive 5M RoPE theta.
struct Qwen3Instruct2507Config : public ModelConfig
{
    static constexpr uint32_t kDefaultContextLength     = 262144;
    static constexpr uint32_t kDefaultEmbeddingLength   = 2560;
    static constexpr uint32_t kDefaultBlockCount        = 36;
    static constexpr uint32_t kDefaultIntermediate      = 9728;
    static constexpr uint32_t kDefaultVocabSize         = 151936;
    static constexpr uint32_t kDefaultHeadCount         = 32;
    static constexpr uint32_t kDefaultHeadCountKv       = 8;
    static constexpr uint32_t kDefaultHeadDim           = 128;

    Qwen3Instruct2507Config()
    {
        // Configure the inner transformer node with Qwen3-4B-Instruct-2507 specs
        m_archConfig.m_arch                     = ModelArchitecture::Qwen3;
        m_archConfig.m_archName                 = "qwen3";
        m_archConfig.m_modelName                = "Qwen3-4B-Instruct-2507";
        m_transformerConfig.m_contextLength     = kDefaultContextLength;
        m_transformerConfig.m_embeddingLength   = kDefaultEmbeddingLength;
        m_transformerConfig.m_blockCount        = kDefaultBlockCount;
        m_transformerConfig.m_feedForwardLength = kDefaultIntermediate;
        m_transformerConfig.m_vocabSize         = kDefaultVocabSize;
        m_transformerConfig.m_headCount         = kDefaultHeadCount;
        m_transformerConfig.m_headCountKv       = kDefaultHeadCountKv;
        m_transformerConfig.m_keyLength         = kDefaultHeadDim;
        m_transformerConfig.m_valueLength       = kDefaultHeadDim;
        m_transformerConfig.m_rmsNormEps        = 1e-6f;
        m_transformerConfig.m_ropeFreqBase      = 5000000.0f; // 5M theta for handling 256k tokens
    }

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // Enforce exact structural invariants for the Qwen3-4B variant.
        return ModelConfig::isValid() &&
               m_archConfig.m_arch == ModelArchitecture::Qwen3 &&
               m_transformerConfig.m_embeddingLength == kDefaultEmbeddingLength &&
               m_transformerConfig.m_blockCount == kDefaultBlockCount &&
               m_transformerConfig.m_vocabSize == kDefaultVocabSize;
    }
};

} // namespace job::model::qwen