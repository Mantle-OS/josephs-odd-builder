#include "model_config_reader_utils.h"

namespace job::model {

void populateModelConfigFromHfJson(const nlohmann::json &j,
                                   const std::string &modelDirName,
                                   ModelConfig &config)
{
    auto getVal = [&](const std::string &key, auto defaultValue) {
        return jsonValueOr(j, key, defaultValue);
    };

    auto &arch = config.archConfig();
    const std::string archStr = getVal("model_type", std::string{"unknown"});

    arch.setArch(stringToModelArchitecture(archStr));
    arch.setArchName(archStr);
    arch.setModelName(modelDirName);
    arch.setHiddenActivation(getVal("hidden_act", std::string{"silu"}));

    auto &transformer = config.transformerConfig();
    transformer.setContextLength(getVal("max_position_embeddings", 0u));
    transformer.setEmbeddingLength(getVal("hidden_size", 0u));
    transformer.setBlockCount(getVal("num_hidden_layers", 0u));
    transformer.setVocabSize(getVal("vocab_size", 0u));

    auto &attention = config.attentionConfig();
    attention.setHeadCount(getVal("num_attention_heads", 0u));
    attention.setHeadCountKv(getVal("num_key_value_heads", attention.headCount()));
    attention.setKeyLength(getVal("head_dim", 0u));
    attention.setValueLength(getVal("head_dim", 0u));
    attention.setAttentionBias(getVal("attention_bias", false));
    attention.setSlidingWindowSize(getVal("sliding_window", 0u));

    config.normConfig().setRmsNormEps(getVal("rms_norm_eps", 1e-5f));
    config.ropeConfig().setRopeFreqBase(getVal("rope_theta", 10000.0f));
    config.feedForwardConfig().setFeedForwardLength(getVal("intermediate_size", 0u));
    config.outputHeadConfig().setTieWordEmbeddings(getVal("tie_word_embeddings", false));
}

void populateSamplerFromHfGenerationConfig(const nlohmann::json &j,
                                           ModelConfig &config)
{
    auto getVal = [&](const std::string &key, auto defaultValue) {
        return jsonValueOr(j, key, defaultValue);
    };

    auto &sampler = config.samplerConfig();
    sampler.setTemperature(getVal("temperature", 0.8f));
    sampler.setTopK(getVal("top_k", 40));
    sampler.setTopP(getVal("top_p", 0.95f));

    if (j.contains("do_sample") && !j["do_sample"].is_null())
        sampler.setGreedy(!j["do_sample"].get<bool>());
}

} // namespace job::model::io_util