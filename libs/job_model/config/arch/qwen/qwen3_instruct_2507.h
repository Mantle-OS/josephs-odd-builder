#pragma once

#include <cstdint>

#include "model_config.h"

namespace job::model::arch::qwen {

// Official model preset for Qwen3-4B-Instruct-2507.
//
// This class only describes model configuration. Graph composition lives
// in QwenGraphBuilder, weight loading lives in ModelWeights/readers, and
// device/runtime policy lives below the model layer.
class Qwen3Instruct2507Config final : public ModelConfig
{
public:
    static constexpr uint32_t kDefaultContextLength   = 262144;
    static constexpr uint32_t kDefaultEmbeddingLength = 2560;
    static constexpr uint32_t kDefaultBlockCount      = 36;
    static constexpr uint32_t kDefaultIntermediate    = 9728;
    static constexpr uint32_t kDefaultVocabSize       = 151936;
    static constexpr uint32_t kDefaultHeadCount       = 32;
    static constexpr uint32_t kDefaultHeadCountKv     = 8;
    static constexpr uint32_t kDefaultHeadDimension   = 128;

    static constexpr float kDefaultRmsNormEps = 1e-6f;
    static constexpr float kDefaultRopeTheta  = 5000000.0f;

    Qwen3Instruct2507Config()
    {
        auto &arch = archConfig();
        arch.setArch(ModelArchitecture::Qwen3);
        arch.setArchName("qwen3");
        arch.setModelName("Qwen3-4B-Instruct-2507");
        arch.setHiddenActivation("silu");

        auto &transformer = transformerConfig();
        transformer.setContextLength(kDefaultContextLength);
        transformer.setEmbeddingLength(kDefaultEmbeddingLength);
        transformer.setBlockCount(kDefaultBlockCount);
        transformer.setVocabSize(kDefaultVocabSize);

        auto &attention = attentionConfig();
        attention.setHeadCount(kDefaultHeadCount);
        attention.setHeadCountKv(kDefaultHeadCountKv);
        attention.setKeyLength(kDefaultHeadDimension);
        attention.setValueLength(kDefaultHeadDimension);
        attention.setAttentionBias(false);
        attention.setAttnLogitSoftCapping(0.0f);
        attention.setSlidingWindowSize(0);

        auto &norm = normConfig();
        norm.setRmsNormEps(kDefaultRmsNormEps);

        auto &rope = ropeConfig();
        rope.setRopeDimensionCount(kDefaultHeadDimension);
        rope.setRopeFreqBase(kDefaultRopeTheta);
        rope.setRopeFreqScale(1.0f);

        auto &ffn = feedForwardConfig();
        ffn.setFeedForwardLength(kDefaultIntermediate);

        auto &output = outputHeadConfig();
        output.setFinalLogitSoftCapping(0.0f);
        output.setTieWordEmbeddings(true);

        clearMoeConfig();
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        if (!ModelConfig::isValid())
            return false;

        const auto &arch        = archConfig();
        const auto &transformer = transformerConfig();
        const auto &attention   = attentionConfig();
        const auto &norm        = normConfig();
        const auto &rope        = ropeConfig();
        const auto &ffn         = feedForwardConfig();

        return
            arch.arch() == ModelArchitecture::Qwen3 &&
            transformer.contextLength() == kDefaultContextLength &&
            transformer.embeddingLength() == kDefaultEmbeddingLength &&
            transformer.blockCount() == kDefaultBlockCount &&
            transformer.vocabSize() == kDefaultVocabSize &&
            attention.headCount() == kDefaultHeadCount &&
            attention.headCountKv() == kDefaultHeadCountKv &&
            attention.keyLength() == kDefaultHeadDimension &&
            attention.valueLength() == kDefaultHeadDimension &&
            norm.rmsNormEps() == kDefaultRmsNormEps &&
            rope.ropeFreqBase() == kDefaultRopeTheta &&
            ffn.feedForwardLength() == kDefaultIntermediate &&
            !hasMoeConfig();
    }
};

} // namespace job::model::arch::qwen