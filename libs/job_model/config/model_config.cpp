#include "config/model_config.h"

#include <algorithm>
#include <vector>
#include <string_view>
#include <job_logger.h>

namespace job::model {

void ModelConfig::clear() noexcept
{
    m_arch = ModelArchitecture::Unknown;
    m_archName.clear();
    m_name.clear();

    m_contextLength = 4096;
    m_embeddingLength = 4096;
    m_blockCount = 32;
    m_feedForwardLength = 11008;
    m_vocabSize = 32000;

    m_headCount = 32;
    m_headCountKv = 32;
    m_keyLength = 0;
    m_valueLength = 0;

    m_rmsNormEps = 1e-5f;
    m_layerNormEps = 1e-5f;

    m_ropeDimensionCount = 0;
    m_ropeFreqBase = 10000.0f;
    m_ropeFreqScale = 1.0f;
    m_ropeScalingType = RopeScalingType::None;
    m_ropeScalingFactor = 1.0f;

    m_expertCount = 0;
    m_expertUsedCount = 0;

    m_finalLogitSoftCapping = 0.0f;
    m_attnLogitSoftCapping = 0.0f;
    m_slidingWindowSize = 0;
}

bool ModelConfig::loadFromFile(const std::filesystem::path& ggufPath)
{
    clear();

    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR("[ModelConfig] GGUF model file does not exist: '{}'", ggufPath.string());
        return false;
    }

    ggml::JobGguf gguf;
    if (!gguf.open(ggufPath)) {
        JOB_LOG_ERROR("[ModelConfig] Failed to open GGUF file '{}': {}", ggufPath.string(), gguf.errorString());
        return false;
    }

    return loadFromGguf(gguf);
}

bool ModelConfig::loadFromGguf(const ggml::JobGguf& gguf)
{
    clear();

    if (!gguf.isValid()) {
        JOB_LOG_ERROR("[ModelConfig] Cannot load configuration: JobGguf instance is invalid");
        return false;
    }

    // Direct framework helper lambdas using JobGgufKv
    auto readInt = [&](const std::string& key, int64_t def) -> int64_t {
        if (!gguf.hasKey(key)) return def;
        auto kv = gguf.keyValue(key);
        if (!kv || !kv->isScalar()) return def;

        switch (kv->type()) {
        case ggml::JobGgufType::UInt8:  return static_cast<int64_t>(kv->value<uint8_t>());
        case ggml::JobGgufType::Int8:   return static_cast<int64_t>(kv->value<int8_t>());
        case ggml::JobGgufType::UInt16: return static_cast<int64_t>(kv->value<uint16_t>());
        case ggml::JobGgufType::Int16:  return static_cast<int64_t>(kv->value<int16_t>());
        case ggml::JobGgufType::UInt32: return static_cast<int64_t>(kv->value<uint32_t>());
        case ggml::JobGgufType::Int32:  return static_cast<int64_t>(kv->value<int32_t>());
        case ggml::JobGgufType::UInt64: return static_cast<int64_t>(kv->value<uint64_t>());
        case ggml::JobGgufType::Int64:  return kv->value<int64_t>();
        default:                        return def;
        }
    };

    auto readFloat = [&](const std::string& key, float def) -> float {
        if (!gguf.hasKey(key)) return def;
        auto kv = gguf.keyValue(key);
        if (!kv || !kv->isScalar()) return def;

        if (kv->type() == ggml::JobGgufType::Float32) {
            return kv->value<float>();
        }
        if (kv->type() == ggml::JobGgufType::Float64) {
            return static_cast<float>(kv->value<double>());
        }
        return def;
    };

    auto readString = [&](const std::string& key, const std::string& def) -> std::string {
        if (!gguf.hasKey(key)) return def;
        auto kv = gguf.keyValue(key);
        if (kv && kv->isScalar() && kv->isString()) {
            return kv->value<std::string>();
        }
        return def;
    };

    // 1. Resolve Architecture
    if (gguf.hasKey("general.architecture")) {
        auto archKv = gguf.keyValue("general.architecture");
        if (archKv && archKv->isString()) {
            m_archName = archKv->value<std::string>();
        }
    }

    if (m_archName.empty()) {
        const std::vector<std::string> candidateArches = {"qwen3", "qwen2", "qwen", "llama", "gemma", "gemma2", "mistral", "phi3", "starcoder2", "chatglm"};
        for (const auto& arch : candidateArches) {
            if (gguf.hasKey(arch + ".block_count") || gguf.hasKey(arch + ".embedding_length") || gguf.hasKey(arch + ".context_length")) {
                m_archName = arch;
                break;
            }
        }
    }

    if (m_archName.empty()) {
        m_archName = "llama";
    }

    m_arch = stringToModelArchitecture(m_archName);
    if (m_arch == ModelArchitecture::Unknown) {
        if (m_archName.find("qwen") != std::string::npos) {
            m_arch = ModelArchitecture::Qwen2; // Map qwen3 / qwen variants to Qwen2 compatibility layer
        } else {
            m_arch = ModelArchitecture::Llama;
        }
    }
    m_name = readString("general.name", m_archName);

    const std::string prefix = m_archName + ".";

    // Helper with multi-prefix fallback for config keys (checking qwen3, qwen2, llama, etc.)
    auto readIntWithFallback = [&](const std::string& keySuffix, int64_t def) -> int64_t {
        std::string primary = prefix + keySuffix;
        if (gguf.hasKey(primary)) return readInt(primary, def);

        const std::vector<std::string> fallbackPrefixes = {"qwen3.", "qwen2.", "llama.", "gemma2.", "mistral.", "phi3.", "general."};
        for (const auto& fp : fallbackPrefixes) {
            std::string k = fp + keySuffix;
            if (gguf.hasKey(k)) return readInt(k, def);
        }
        return def;
    };

    auto readFloatWithFallback = [&](const std::string& keySuffix, float def) -> float {
        std::string primary = prefix + keySuffix;
        if (gguf.hasKey(primary)) return readFloat(primary, def);

        const std::vector<std::string> fallbackPrefixes = {"qwen3.", "qwen2.", "llama.", "gemma2.", "mistral.", "phi3.", "general."};
        for (const auto& fp : fallbackPrefixes) {
            std::string k = fp + keySuffix;
            if (gguf.hasKey(k)) return readFloat(k, def);
        }
        return def;
    };

    auto readStringWithFallback = [&](const std::string& keySuffix, const std::string& def) -> std::string {
        std::string primary = prefix + keySuffix;
        if (gguf.hasKey(primary)) return readString(primary, def);

        const std::vector<std::string> fallbackPrefixes = {"qwen3.", "qwen2.", "llama.", "gemma2.", "mistral.", "phi3.", "general."};
        for (const auto& fp : fallbackPrefixes) {
            std::string k = fp + keySuffix;
            if (gguf.hasKey(k)) return readString(k, def);
        }
        return def;
    };

    // Transformer Dimensions
    m_contextLength     = static_cast<uint32_t>(readIntWithFallback("context_length", 4096));
    m_embeddingLength   = static_cast<uint32_t>(readIntWithFallback("embedding_length", 4096));
    m_blockCount        = static_cast<uint32_t>(readIntWithFallback("block_count", 32));
    m_feedForwardLength = static_cast<uint32_t>(readIntWithFallback("feed_forward_length", 0));

    if (m_feedForwardLength == 0 && m_embeddingLength > 0) {
        m_feedForwardLength = static_cast<uint32_t>((4 * m_embeddingLength * 2) / 3);
        m_feedForwardLength = ((m_feedForwardLength + 255) / 256) * 256;
    }

    // 3. Vocab Size
    std::string vocabKey = prefix + "vocab_size";
    if (gguf.hasKey(vocabKey)) {
        m_vocabSize = static_cast<uint32_t>(readInt(vocabKey, 32000));
    } else if (gguf.hasKey("qwen3.vocab_size")) {
        m_vocabSize = static_cast<uint32_t>(readInt("qwen3.vocab_size", 32000));
    } else if (gguf.hasKey("qwen2.vocab_size")) {
        m_vocabSize = static_cast<uint32_t>(readInt("qwen2.vocab_size", 32000));
    } else if (gguf.hasKey("llama.vocab_size")) {
        m_vocabSize = static_cast<uint32_t>(readInt("llama.vocab_size", 32000));
    } else if (gguf.hasKey("tokenizer.ggml.tokens")) {
        auto tokensKv = gguf.keyValue("tokenizer.ggml.tokens");
        if (tokensKv) {
            m_vocabSize = static_cast<uint32_t>(tokensKv->elementCount());
        }
    }

    // Attention Heads
    m_headCount   = static_cast<uint32_t>(readIntWithFallback("attention.head_count", 32));
    m_headCountKv = static_cast<uint32_t>(readIntWithFallback("attention.head_count_kv", m_headCount));

    m_keyLength   = static_cast<uint32_t>(readIntWithFallback("attention.key_length", 0));
    m_valueLength = static_cast<uint32_t>(readIntWithFallback("attention.value_length", 0));

    if (m_keyLength == 0 && m_headCount > 0) {
        m_keyLength = m_embeddingLength / m_headCount;
    }
    if (m_valueLength == 0) {
        m_valueLength = m_keyLength;
    }

    // 5. Normalization Epsilon
    m_rmsNormEps   = readFloatWithFallback("attention.layer_norm_rms_epsilon", 1e-5f);
    m_layerNormEps = readFloatWithFallback("attention.layer_norm_epsilon", 1e-5f);

    // 6. RoPE
    m_ropeDimensionCount = static_cast<uint32_t>(readIntWithFallback("rope.dimension_count", m_keyLength));
    m_ropeFreqBase       = readFloatWithFallback("rope.freq_base", 10000.0f);
    m_ropeFreqScale      = readFloatWithFallback("rope.freq_scale", 1.0f);

    std::string ropeScaling = readStringWithFallback("rope.scaling.type", "");
    if (ropeScaling == "linear") {
        m_ropeScalingType = RopeScalingType::Linear;
    } else if (ropeScaling == "yarn") {
        m_ropeScalingType = RopeScalingType::Yarn;
    } else if (ropeScaling == "dynamic") {
        m_ropeScalingType = RopeScalingType::Dynamic;
    } else if (ropeScaling == "longrope") {
        m_ropeScalingType = RopeScalingType::LongRoPE;
    }
    m_ropeScalingFactor = readFloatWithFallback("rope.scaling.factor", 1.0f);

    // 7. Mixture of Experts (MoE)
    m_expertCount     = static_cast<uint32_t>(readIntWithFallback("expert_count", 0));
    m_expertUsedCount = static_cast<uint32_t>(readIntWithFallback("expert_used_count", 0));

    // 8. Soft-capping & Sliding Window
    m_finalLogitSoftCapping = readFloatWithFallback("final_logit_softcapping", 0.0f);
    m_attnLogitSoftCapping  = readFloatWithFallback("attention.logit_softcapping", 0.0f);
    m_slidingWindowSize     = static_cast<uint32_t>(readIntWithFallback("attention.sliding_window", 0));

    // JOB_LOG_INFO("[ModelConfig] Loaded {} model '{}' (Layers: {}, Embd: {}, Heads: {}/{}, Ctx: {})",
    //              m_archName, m_name, m_blockCount, m_embeddingLength, m_headCount, m_headCountKv, m_contextLength);

    return true;
}

uint32_t ModelConfig::headDimension() const noexcept
{
    if (m_keyLength > 0) return m_keyLength;
    if (m_headCount > 0) return m_embeddingLength / m_headCount;
    return 0;
}

uint32_t ModelConfig::headDimensionKv() const noexcept
{
    if (m_valueLength > 0) return m_valueLength;
    return headDimension();
}

ggml::JobGgmlVDShape ModelConfig::tokenEmbeddingShape() const noexcept
{
    return ggml::JobGgmlVDShape{
        .vocabulary = static_cast<int64_t>(m_vocabSize),
        .dimension  = static_cast<int64_t>(m_embeddingLength)
    };
}

ggml::JobGgmlLinearShape ModelConfig::qProjectionShape() const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(m_headCount * headDimension()),
        .inputDimension  = static_cast<int64_t>(m_embeddingLength)
    };
}

ggml::JobGgmlLinearShape ModelConfig::kProjectionShape() const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(m_headCountKv * headDimensionKv()),
        .inputDimension  = static_cast<int64_t>(m_embeddingLength)
    };
}

ggml::JobGgmlLinearShape ModelConfig::vProjectionShape() const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(m_headCountKv * headDimensionKv()),
        .inputDimension  = static_cast<int64_t>(m_embeddingLength)
    };
}

ggml::JobGgmlLinearShape ModelConfig::outProjectionShape() const noexcept
{
    return ggml::JobGgmlLinearShape{
        .outputDimension = static_cast<int64_t>(m_embeddingLength),
        .inputDimension  = static_cast<int64_t>(m_headCount * headDimension())
    };
}

ggml::JobGgmlBSHDShape ModelConfig::qActivationShape(int64_t batch, int64_t seq) const noexcept
{
    return ggml::JobGgmlBSHDShape{
        .batch         = batch,
        .sequence      = seq,
        .heads         = static_cast<int64_t>(m_headCount),
        .headDimension = static_cast<int64_t>(headDimension())
    };
}

ggml::JobGgmlBSHDShape ModelConfig::kvActivationShape(int64_t batch, int64_t seq) const noexcept
{
    return ggml::JobGgmlBSHDShape{
        .batch         = batch,
        .sequence      = seq,
        .heads         = static_cast<int64_t>(m_headCountKv),
        .headDimension = static_cast<int64_t>(headDimensionKv())
    };
}

} // namespace job::model