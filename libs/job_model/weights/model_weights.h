#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "config/model_config.h"
#include "job_ggml_context.h"
#include "job_ggml_tensor.h"
#include "job_gguf.h"
#include "jobmodel_export.h"

namespace job::model {

struct JOBMODEL_EXPORT LayerWeights {
    uint32_t layerIndex{0};

    // Attention Input Norm
    ggml::JobGgmlTensor::UPtr attnNorm;
    ggml::JobGgmlTensor::UPtr attnNormBias;

    // Attention Projections (Q, K, V, Output)
    ggml::JobGgmlTensor::UPtr attnQ;
    ggml::JobGgmlTensor::UPtr attnK;
    ggml::JobGgmlTensor::UPtr attnV;
    ggml::JobGgmlTensor::UPtr attnOut;

    // Attention Biases (Qwen / Phi / StarCoder)
    ggml::JobGgmlTensor::UPtr attnQBias;
    ggml::JobGgmlTensor::UPtr attnKBias;
    ggml::JobGgmlTensor::UPtr attnVBias;
    ggml::JobGgmlTensor::UPtr attnOutBias;

    // Q/K LayerNorms (Gemma 2 / Command-R)
    ggml::JobGgmlTensor::UPtr attnQNorm;
    ggml::JobGgmlTensor::UPtr attnKNorm;

    // Post-Attention Norm (Gemma 2)
    ggml::JobGgmlTensor::UPtr postAttnNorm;

    // Feed-Forward Input Norm
    ggml::JobGgmlTensor::UPtr ffnNorm;
    ggml::JobGgmlTensor::UPtr ffnNormBias;

    // Dense Feed-Forward Projections (SwiGLU / GeLU)
    ggml::JobGgmlTensor::UPtr ffnGate; // w1 / gate_proj
    ggml::JobGgmlTensor::UPtr ffnUp;   // w3 / up_proj
    ggml::JobGgmlTensor::UPtr ffnDown; // w2 / down_proj

    // Feed-Forward Biases
    ggml::JobGgmlTensor::UPtr ffnGateBias;
    ggml::JobGgmlTensor::UPtr ffnUpBias;
    ggml::JobGgmlTensor::UPtr ffnDownBias;

    // Post-FFN Norm (Gemma 2)
    ggml::JobGgmlTensor::UPtr postFfnNorm;

    // Mixture of Experts (MoE)
    ggml::JobGgmlTensor::UPtr ffnGateInp; // Router / Gating tensor
    ggml::JobGgmlTensor::UPtr ffnGateExps;
    ggml::JobGgmlTensor::UPtr ffnUpExps;
    ggml::JobGgmlTensor::UPtr ffnDownExps;
};

class JOBMODEL_EXPORT ModelWeights {
public:
    ModelWeights() = default;
    ~ModelWeights() = default;

    ModelWeights(const ModelWeights&) = delete;
    ModelWeights& operator=(const ModelWeights&) = delete;
    ModelWeights(ModelWeights&&) noexcept = default;
    ModelWeights& operator=(ModelWeights&&) noexcept = default;

    // Bind weights resolved from JobGgmlContext holding loaded GGUF tensors
    [[nodiscard]] bool loadFromContext(ggml::JobGgmlContext& context, const ModelConfig& config);

    void clear() noexcept;
    [[nodiscard]] bool isLoaded() const noexcept { return m_tokenEmbd != nullptr && m_tokenEmbd->isValid(); }

    // Global / Transformer-level Tensors
    [[nodiscard]] const ggml::JobGgmlTensor* tokenEmbd() const noexcept { return m_tokenEmbd.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor* outputNorm() const noexcept { return m_outputNorm.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor* outputNormBias() const noexcept { return m_outputNormBias.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor* output() const noexcept { return m_output ? m_output.get() : m_tokenEmbd.get(); }
    [[nodiscard]] bool hasTiedEmbedding() const noexcept { return m_output == nullptr; }

    // Per-Layer Weights
    [[nodiscard]] size_t layerCount() const noexcept { return m_layers.size(); }
    [[nodiscard]] const LayerWeights& layer(size_t index) const;
    [[nodiscard]] const std::vector<LayerWeights>& layers() const noexcept { return m_layers; }

    // Optional Vision / Multimodal Embeddings
    [[nodiscard]] const ggml::JobGgmlTensor* positionEmbd() const noexcept { return m_positionEmbd.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor* typeEmbd() const noexcept { return m_typeEmbd.get(); }

private:
    [[nodiscard]] ggml::JobGgmlTensor::UPtr findTensor(
        ggml::JobGgmlContext& context,
        std::string_view canonicalName,
        std::initializer_list<std::string_view> fallbackAliases = {}) const;

private:
    // Global Weights
    ggml::JobGgmlTensor::UPtr m_tokenEmbd;
    ggml::JobGgmlTensor::UPtr m_outputNorm;
    ggml::JobGgmlTensor::UPtr m_outputNormBias;
    ggml::JobGgmlTensor::UPtr m_output;

    ggml::JobGgmlTensor::UPtr m_positionEmbd;
    ggml::JobGgmlTensor::UPtr m_typeEmbd;

    // Transformer Blocks
    std::vector<LayerWeights> m_layers;
};

} // namespace job::model