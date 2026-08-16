#include "model_config_reader.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include <job_logger.h>
#include <job_gguf_kv.h>

namespace job::model {

bool ModelConfigReader::readFromGguf(const ggml::JobGguf& gguf, ModelConfig& config)
{
    // Reset config to a clean slate before reading
    config = ModelConfig{};

    // 1. Read General Architecture Metadata
    std::string archStr = "unknown";
    if (gguf.hasKey("general.architecture")) {
        try {
            auto kv = gguf.keyValue("general.architecture");
            if (kv && kv->isValid() && kv->isString()) {
                archStr = kv->value<std::string>();
            }
        } catch (...) {
            // Fallback handled below
        }
    }

    if (archStr == "unknown") {
        JOB_LOG_ERROR("[ModelConfigReader] Missing required 'general.architecture' key in GGUF metadata");
        return false;
    }

    config.m_archConfig.m_arch = stringToModelArchitecture(archStr);
    config.m_archConfig.m_archName = archStr;

    if (gguf.hasKey("general.name")) {
        try {
            auto kv = gguf.keyValue("general.name");
            if (kv && kv->isValid() && kv->isString()) {
                config.m_archConfig.m_modelName = kv->value<std::string>();
            }
        } catch (...) {
            config.m_archConfig.m_modelName = "unknown_model";
        }
    } else {
        config.m_archConfig.m_modelName = "unknown_model";
    }

    // Helper lambda to safely query architecture-prefixed keys with general fallbacks using JobGguf API
    auto getValU32 = [&](std::string_view keySuffix, uint32_t defaultValue) -> uint32_t {
        std::string archKey = archStr + std::string(".") + std::string(keySuffix);
        if (gguf.hasKey(archKey)) {
            try {
                auto kv = gguf.keyValue(archKey);
                if (kv && kv->isValid() && kv->isInteger()) {
                    return static_cast<uint32_t>(kv->value<std::int64_t>());
                }
            } catch (...) {}
        }

        std::string genKey = std::string("general.") + std::string(keySuffix);
        if (gguf.hasKey(genKey)) {
            try {
                auto kv = gguf.keyValue(genKey);
                if (kv && kv->isValid() && kv->isInteger()) {
                    return static_cast<uint32_t>(kv->value<std::int64_t>());
                }
            } catch (...) {}
        }

        return defaultValue;
    };

    auto getValFloat = [&](std::string_view keySuffix, float defaultValue) -> float {
        std::string archKey = archStr + std::string(".") + std::string(keySuffix);
        if (gguf.hasKey(archKey)) {
            try {
                auto kv = gguf.keyValue(archKey);
                if (kv && kv->isValid() && kv->isFloatingPoint()) {
                    return static_cast<float>(kv->value<double>());
                }
            } catch (...) {}
        }
        return defaultValue;
    };

    // 2. Read Transformer Dimensions
    config.m_transformerConfig.m_contextLength     = getValU32("context_length", 4096);
    config.m_transformerConfig.m_embeddingLength   = getValU32("embedding_length", 4096);
    config.m_transformerConfig.m_blockCount        = getValU32("block_count", 32);
    config.m_transformerConfig.m_feedForwardLength = getValU32("feed_forward_length", 11008);
    config.m_transformerConfig.m_vocabSize         = getValU32("vocab_size", 32000);

    // 3. Read Attention Geometry
    config.m_transformerConfig.m_headCount     = getValU32("attention.head_count", 32);
    config.m_transformerConfig.m_headCountKv   = getValU32("attention.head_count_kv", config.m_transformerConfig.m_headCount);
    config.m_transformerConfig.m_keyLength     = getValU32("attention.key_length", 0);
    config.m_transformerConfig.m_valueLength   = getValU32("attention.value_length", 0);

    // 4. Read Normalization & RoPE
    config.m_transformerConfig.m_rmsNormEps      = getValFloat("attention.layer_norm_rms_epsilon", 1e-5f);
    config.m_transformerConfig.m_ropeDimensionCount = getValU32("rope.dimension_count", 0);
    config.m_transformerConfig.m_ropeFreqBase    = getValFloat("rope.freq_base", 10000.0f);

    // 5. Read Architectural Quirks (Soft-capping & Sliding Window)
    config.m_archConfig.m_finalLogitSoftCapping = getValFloat("final_logit_softcapping", 0.0f);
    config.m_archConfig.m_attnLogitSoftCapping  = getValFloat("attention.logit_softcapping", 0.0f);
    config.m_archConfig.m_slidingWindowSize     = getValU32("attention.sliding_window", 0);

    if (!config.isValid()) {
        JOB_LOG_ERROR("[ModelConfigReader] Loaded configuration failed validation checks for model '{}'", config.m_archConfig.m_modelName);
        return false;
    }

    return true;
}

bool ModelConfigReader::readFromFile(const std::filesystem::path& ggufPath, ModelConfig& config)
{
    if (!std::filesystem::exists(ggufPath)) {
        JOB_LOG_ERROR("[ModelConfigReader] GGUF file path does not exist: '{}'", ggufPath.string());
        return false;
    }

    ggml::JobGgmlContext::UPtr tempWeightCtx;
    ggml::JobGguf gguf(&tempWeightCtx);

    if (!gguf.open(ggufPath)) {
        JOB_LOG_ERROR("[ModelConfigReader] Failed to open GGUF file for reading config: {}", gguf.errorString());
        return false;
    }

    return readFromGguf(gguf, config);
}
bool ModelConfigReader::readFromJsonDirectory(const std::filesystem::path& dirPath, ModelConfig& config)
{
    config = ModelConfig{};

    auto configPath = dirPath / "config.json";
    if (!std::filesystem::exists(configPath)) {
        JOB_LOG_ERROR("[ModelConfigReader] config.json not found in directory: '{}'", dirPath.string());
        return false;
    }

    try {
        std::ifstream f(configPath);
        nlohmann::json j = nlohmann::json::parse(f);

        // Safe helper lambda that ignores explicit JSON 'null' values and missing keys
        auto getVal = [&](const std::string& key, auto defaultValue) {
            using T = decltype(defaultValue);
            if (j.contains(key) && !j[key].is_null()) {
                try {
                    return j[key].get<T>();
                } catch (...) {
                    return defaultValue;
                }
            }
            return defaultValue;
        };

        // 1. Architecture Metadata & Quirks (ArchConfig)
        std::string archStr = getVal("model_type", std::string("unknown"));
        config.m_archConfig.m_arch                 = stringToModelArchitecture(archStr);
        config.m_archConfig.m_archName             = archStr;
        config.m_archConfig.m_modelName            = dirPath.filename().string();
        config.m_archConfig.m_attentionBias        = getVal("attention_bias", false);
        config.m_archConfig.m_tieWordEmbeddings    = getVal("tie_word_embeddings", false);
        config.m_archConfig.m_hiddenActivation     = getVal("hidden_act", std::string("silu"));
        config.m_archConfig.m_slidingWindowSize    = getVal("sliding_window", 0u);

        // 2. Core Transformer Dimensions & Geometry (TransformerConfig)
        config.m_transformerConfig.m_contextLength     = getVal("max_position_embeddings", 4096u);
        config.m_transformerConfig.m_embeddingLength   = getVal("hidden_size", 4096u);
        config.m_transformerConfig.m_blockCount        = getVal("num_hidden_layers", 32u);
        config.m_transformerConfig.m_feedForwardLength = getVal("intermediate_size", 11008u);
        config.m_transformerConfig.m_vocabSize         = getVal("vocab_size", 32000u);

        config.m_transformerConfig.m_headCount         = getVal("num_attention_heads", 32u);
        config.m_transformerConfig.m_headCountKv       = getVal("num_key_value_heads", config.m_transformerConfig.m_headCount);
        config.m_transformerConfig.m_keyLength         = getVal("head_dim", 0u);
        config.m_transformerConfig.m_valueLength       = getVal("head_dim", 0u);

        config.m_transformerConfig.m_rmsNormEps        = getVal("rms_norm_eps", 1e-5f);
        config.m_transformerConfig.m_ropeFreqBase      = getVal("rope_theta", 10000.0f);

    } catch (const std::exception& e) {
        JOB_LOG_ERROR("[ModelConfigReader] Failed to parse config.json: {}", e.what());
        return false;
    }

    // 3. Optional Generation Parameters (SamplerConfig)
    auto genConfigPath = dirPath / "generation_config.json";
    if (std::filesystem::exists(genConfigPath)) {
        try {
            std::ifstream gf(genConfigPath);
            nlohmann::json gj = nlohmann::json::parse(gf);

            auto getGenVal = [&](const std::string& key, auto defaultValue) {
                using T = decltype(defaultValue);
                if (gj.contains(key) && !gj[key].is_null()) {
                    try {
                        return gj[key].get<T>();
                    } catch (...) {
                        return defaultValue;
                    }
                }
                return defaultValue;
            };

            config.m_samplerConfig.m_temperature = getGenVal("temperature", 0.8f);
            config.m_samplerConfig.m_topK        = getGenVal("top_k", 40);
            config.m_samplerConfig.m_topP        = getGenVal("top_p", 0.95f);
            if (gj.contains("do_sample") && !gj["do_sample"].is_null()) {
                config.m_samplerConfig.m_greedy  = !gj["do_sample"].get<bool>();
            }
        } catch (const std::exception& e) {
            JOB_LOG_ERROR("[ModelConfigReader] Failed to parse generation_config.json (non-fatal): {}", e.what());
        }
    }

    if (!config.isValid()) {
        JOB_LOG_ERROR("[ModelConfigReader] Parsed JSON configuration failed validation checks");
        return false;
    }

    JOB_LOG_INFO("[ModelConfigReader] Successfully parsed JSON config for '{}' (Arch: {}, Layers: {}, Ctx: {})",
                 config.m_archConfig.m_modelName, config.m_archConfig.m_archName,
                 config.m_transformerConfig.m_blockCount,
                 config.m_transformerConfig.m_contextLength);

    return true;
}
} // namespace job::model