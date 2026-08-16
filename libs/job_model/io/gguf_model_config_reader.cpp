#include "gguf_model_config_reader.h"

#include <stdexcept>

#include <job_logger.h>
#include <job_gguf_kv.h>

#include "model_config_reader_utils.h"

namespace job::model {

bool GgufModelConfigReader::read(const std::filesystem::path &modelPath, ModelConfig &config)
{
    if (!std::filesystem::exists(modelPath)) {
        JOB_LOG_ERROR("[GgufModelConfigReader] GGUF file path does not exist: '{}'", modelPath.string());
        return false;
    }

    ggml::JobGgmlContext::UPtr tempWeightCtx;
    ggml::JobGguf gguf(&tempWeightCtx);

    if (!gguf.open(modelPath)) {
        JOB_LOG_ERROR("[GgufModelConfigReader] Failed to open GGUF file for reading config: {}", gguf.errorString());
        return false;
    }

    return readFromGguf(gguf, config);
}

bool GgufModelConfigReader::readFromGguf(const ggml::JobGguf &gguf, ModelConfig &config)
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
        JOB_LOG_ERROR("[GgufModelConfigReader] Missing required 'general.architecture' key in GGUF metadata");
        return false;
    }

    config.archConfig().setArch(stringToModelArchitecture(archStr));
    config.archConfig().setArchName(archStr);

    if (gguf.hasKey("general.name")) {
        try {
            auto kv = gguf.keyValue("general.name");
            if (kv && kv->isValid() && kv->isString()) {
                config.archConfig().setModelName(kv->value<std::string>());
            } else {
                config.archConfig().setModelName("unknown_model");
            }
        } catch (...) {
            config.archConfig().setModelName("unknown_model");
        }
    } else {
        config.archConfig().setModelName("unknown_model");
    }

    auto getValU32 = [&](std::string_view keySuffix, uint32_t defaultValue) {
        return io_util::ggufValueU32(gguf, archStr, keySuffix, defaultValue);
    };
    auto getValFloat = [&](std::string_view keySuffix, float defaultValue) {
        return io_util::ggufValueFloat(gguf, archStr, keySuffix, defaultValue);
    };

    try {
        // 2. Read Transformer Dimensions (genuinely global -- see TransformerConfig)
        auto &transformer = config.transformerConfig();
        transformer.setContextLength(getValU32("context_length", 4096));
        transformer.setEmbeddingLength(getValU32("embedding_length", 4096));
        transformer.setBlockCount(getValU32("block_count", 32));
        transformer.setVocabSize(getValU32("vocab_size", 32000));

        // 3. Read Attention Geometry (MHA / GQA / MQA + windowing/bias/softcap)
        auto &attention = config.attentionConfig();
        attention.setHeadCount(getValU32("attention.head_count", 32));
        attention.setHeadCountKv(getValU32("attention.head_count_kv", attention.headCount()));
        attention.setKeyLength(getValU32("attention.key_length", 0));
        attention.setValueLength(getValU32("attention.value_length", 0));
        attention.setAttnLogitSoftCapping(getValFloat("attention.logit_softcapping", 0.0f));
        attention.setSlidingWindowSize(getValU32("attention.sliding_window", 0));

        // 4. Read Normalization
        config.normConfig().setRmsNormEps(getValFloat("attention.layer_norm_rms_epsilon", 1e-5f));

        // 5. Read RoPE
        auto &rope = config.ropeConfig();
        rope.setRopeDimensionCount(getValU32("rope.dimension_count", 0));
        rope.setRopeFreqBase(getValFloat("rope.freq_base", 10000.0f));

        // 6. Read Feed-Forward dimension
        config.feedForwardConfig().setFeedForwardLength(getValU32("feed_forward_length", 11008));

        // 7. Read Output-Head quirks (soft-capping). Note: tieWordEmbeddings
        // was never read from GGUF metadata historically -- preserved as-is;
        // ModelWeights::loadFromContext() derives it from tensor presence instead.
        config.outputHeadConfig().setFinalLogitSoftCapping(getValFloat("final_logit_softcapping", 0.0f));
    } catch (const std::invalid_argument &e) {
        JOB_LOG_ERROR("[GgufModelConfigReader] GGUF metadata contained an out-of-range value: {}", e.what());
        return false;
    }

    return finalizeAndValidate(config, "GgufModelConfigReader");
}

} // namespace job::model