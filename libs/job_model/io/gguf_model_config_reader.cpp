#include "gguf_model_config_reader.h"

#include <stdexcept>
#include <string>

#include <job_logger.h>
#include <job_gguf_kv.h>

namespace job::model {

bool GgufModelConfigReader::read(const std::filesystem::path &modelPath, ModelConfig &config)
{
    config = ModelConfig{};

    if (!std::filesystem::exists(modelPath)) {
        JOB_LOG_ERROR("[GgufModelConfigReader] GGUF file path does not exist: '{}'", modelPath.string());
        return false;
    }

    ggml::JobGgmlContext::UPtr context;
    ggml::JobGguf gguf{&context};

    gguf.initParams()->setNoAlloc(true);
    gguf.initParams()->setCreateContext(false);

    if (!gguf.open(modelPath)) {
        JOB_LOG_ERROR("[GgufModelConfigReader] Failed to open GGUF file for reading config: {}", gguf.errorString());
        return false;
    }

    return readFromGguf(gguf, config);
}

bool GgufModelConfigReader::readFromGguf(const ggml::JobGguf &gguf, ModelConfig &config)
{
    config = ModelConfig{};

    try {
        const auto architectureKv = gguf.keyValue("general.architecture");
        if (!architectureKv)
            throw std::runtime_error{"Missing required GGUF key 'general.architecture'"};

        const std::string architecture = architectureKv->value<std::string>();
        const std::string prefix = architecture + ".";

        auto &arch = config.archConfig();
        arch.setArch(stringToModelArchitecture(architecture));
        arch.setArchName(architecture);

        const auto nameKv = gguf.keyValue("general.name");
        if (!nameKv)
            throw std::runtime_error{"Missing required GGUF key 'general.name'"};

        arch.setModelName(nameKv->value<std::string>());

        auto &transformer = config.transformerConfig();

        if (const auto kv = gguf.keyValue(prefix + "context_length"))
            transformer.setContextLength(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "embedding_length"))
            transformer.setEmbeddingLength(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "block_count"))
            transformer.setBlockCount(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue("tokenizer.ggml.tokens"))
            transformer.setVocabSize(static_cast<uint32_t>(kv->elementCount()));

        auto &attention = config.attentionConfig();

        if (const auto kv = gguf.keyValue(prefix + "attention.head_count"))
            attention.setHeadCount(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "attention.head_count_kv"))
            attention.setHeadCountKv(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "attention.key_length"))
            attention.setKeyLength(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "attention.value_length"))
            attention.setValueLength(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "attention.logit_softcapping"))
            attention.setAttnLogitSoftCapping(kv->value<float>());

        if (const auto kv = gguf.keyValue(prefix + "attention.sliding_window"))
            attention.setSlidingWindowSize(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "attention.layer_norm_rms_epsilon"))
            config.normConfig().setRmsNormEps(kv->value<float>());

        auto &rope = config.ropeConfig();

        if (const auto kv = gguf.keyValue(prefix + "rope.dimension_count"))
            rope.setRopeDimensionCount(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "rope.freq_base"))
            rope.setRopeFreqBase(kv->value<float>());

        if (const auto kv = gguf.keyValue(prefix + "feed_forward_length"))
            config.feedForwardConfig().setFeedForwardLength(kv->value<uint32_t>());

        if (const auto kv = gguf.keyValue(prefix + "final_logit_softcapping"))
            config.outputHeadConfig().setFinalLogitSoftCapping(kv->value<float>());
    }
    catch (const std::exception &e) {
        config = ModelConfig{};
        JOB_LOG_ERROR("[GgufModelConfigReader] Failed to read GGUF model metadata: {}", e.what());
        return false;
    }

    return finalizeAndValidate(config, "GgufModelConfigReader");
}

} // namespace job::model