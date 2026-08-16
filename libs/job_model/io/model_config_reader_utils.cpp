#include "model_config_reader_utils.h"

#include <algorithm>
#include <cctype>

#include <job_gguf_kv.h>

namespace job::model::io_util {

uint32_t ggufValueU32(const ggml::JobGguf &gguf, std::string_view archName,
                      std::string_view keySuffix, uint32_t defaultValue)
{
    const std::string archKey = std::string(archName) + "." + std::string(keySuffix);
    if (gguf.hasKey(archKey)) {
        try {
            auto kv = gguf.keyValue(archKey);
            if (kv && kv->isValid() && kv->isInteger())
                return static_cast<uint32_t>(kv->value<std::int64_t>());
        } catch (...) {}
    }

    const std::string genKey = std::string("general.") + std::string(keySuffix);
    if (gguf.hasKey(genKey)) {
        try {
            auto kv = gguf.keyValue(genKey);
            if (kv && kv->isValid() && kv->isInteger())
                return static_cast<uint32_t>(kv->value<std::int64_t>());
        } catch (...) {}
    }

    return defaultValue;
}

float ggufValueFloat(const ggml::JobGguf &gguf, std::string_view archName,
                     std::string_view keySuffix, float defaultValue)
{
    const std::string archKey = std::string(archName) + "." + std::string(keySuffix);
    if (gguf.hasKey(archKey)) {
        try {
            auto kv = gguf.keyValue(archKey);
            if (kv && kv->isValid() && kv->isFloatingPoint())
                return static_cast<float>(kv->value<double>());
        } catch (...) {}
    }

    return defaultValue;
}

void populateModelConfigFromHfJson(const nlohmann::json &j, const std::string &modelDirName, ModelConfig &config)
{
    auto getVal = [&](const std::string &key, auto defaultValue) {
        return jsonValueOr(j, key, defaultValue);
    };

    // 1. Architecture identity (ArchConfig) -- hiddenActivation stays here
    // per the arch-config audit; a real gating concept doesn't exist yet
    // to justify moving it to FeedForwardConfig.
    std::string archStr = getVal("model_type", std::string("unknown"));
    auto &arch = config.archConfig();
    arch.setArch(stringToModelArchitecture(archStr));
    arch.setArchName(archStr);
    arch.setModelName(modelDirName);
    arch.setHiddenActivation(getVal("hidden_act", std::string("silu")));

    // 2. Core Transformer Dimensions (genuinely global -- see TransformerConfig)
    auto &transformer = config.transformerConfig();
    transformer.setContextLength(getVal("max_position_embeddings", 4096u));
    transformer.setEmbeddingLength(getVal("hidden_size", 4096u));
    transformer.setBlockCount(getVal("num_hidden_layers", 32u));
    transformer.setVocabSize(getVal("vocab_size", 32000u));

    // 3. Attention Geometry + quirks (MHA / GQA / MQA + bias/windowing)
    auto &attention = config.attentionConfig();
    attention.setHeadCount(getVal("num_attention_heads", 32u));
    attention.setHeadCountKv(getVal("num_key_value_heads", attention.headCount()));
    attention.setKeyLength(getVal("head_dim", 0u));
    attention.setValueLength(getVal("head_dim", 0u));
    attention.setAttentionBias(getVal("attention_bias", false));
    attention.setSlidingWindowSize(getVal("sliding_window", 0u));

    // 4. Normalization
    config.normConfig().setRmsNormEps(getVal("rms_norm_eps", 1e-5f));

    // 5. RoPE
    config.ropeConfig().setRopeFreqBase(getVal("rope_theta", 10000.0f));

    // 6. Feed-Forward dimension
    config.feedForwardConfig().setFeedForwardLength(getVal("intermediate_size", 11008u));

    // 7. Output-Head quirks
    config.outputHeadConfig().setTieWordEmbeddings(getVal("tie_word_embeddings", false));
}

void populateSamplerFromHfGenerationConfig(const nlohmann::json &gj, ModelConfig &config)
{
    auto getGenVal = [&](const std::string &key, auto defaultValue) {
        return jsonValueOr(gj, key, defaultValue);
    };

    auto &sampler = config.samplerConfig();
    sampler.setTemperature(getGenVal("temperature", 0.8f));
    sampler.setTopK(getGenVal("top_k", 40));
    sampler.setTopP(getGenVal("top_p", 0.95f));
    if (gj.contains("do_sample") && !gj["do_sample"].is_null()) {
        sampler.setGreedy(!gj["do_sample"].get<bool>());
    }
}

uint32_t estimateBlockCountFromTensorNames(const std::vector<std::string> &tensorNames)
{
    int64_t maxIndex = -1;

    for (const auto &name : tensorNames) {
        for (const std::string_view marker : {".layers.", ".h."}) {
            auto pos = name.find(marker);
            if (pos == std::string::npos)
                continue;

            pos += marker.size();
            const auto end = name.find('.', pos);
            if (end == std::string::npos)
                continue;

            const std::string numStr = name.substr(pos, end - pos);
            if (numStr.empty() || !std::all_of(numStr.begin(), numStr.end(),
                                               [](unsigned char c) { return std::isdigit(c); }))
                continue;

            try {
                maxIndex = std::max(maxIndex, static_cast<int64_t>(std::stoll(numStr)));
            } catch (...) {
                // Numeric segment too large for int64_t or otherwise
                // unparsable -- skip it, not worth failing the whole scan.
            }
        }
    }

    return maxIndex >= 0 ? static_cast<uint32_t>(maxIndex + 1) : 0;
}

} // namespace job::model::io_util