#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <job_ggml_context.h>
#include <job_ggml_tensor.h>

#include "config/model_config.h"
#include "jobmodel_export.h"

namespace job::model {

struct JOBMODEL_EXPORT LayerWeights
{
    uint32_t layerIndex{0};

    ggml::JobGgmlTensor::UPtr attnNorm;
    ggml::JobGgmlTensor::UPtr attnNormBias;

    ggml::JobGgmlTensor::UPtr attnQ;
    ggml::JobGgmlTensor::UPtr attnK;
    ggml::JobGgmlTensor::UPtr attnV;
    ggml::JobGgmlTensor::UPtr attnOut;

    ggml::JobGgmlTensor::UPtr attnQBias;
    ggml::JobGgmlTensor::UPtr attnKBias;
    ggml::JobGgmlTensor::UPtr attnVBias;
    ggml::JobGgmlTensor::UPtr attnOutBias;

    ggml::JobGgmlTensor::UPtr attnQNorm;
    ggml::JobGgmlTensor::UPtr attnKNorm;

    ggml::JobGgmlTensor::UPtr postAttnNorm;

    ggml::JobGgmlTensor::UPtr ffnNorm;
    ggml::JobGgmlTensor::UPtr ffnNormBias;

    ggml::JobGgmlTensor::UPtr ffnGate;
    ggml::JobGgmlTensor::UPtr ffnUp;
    ggml::JobGgmlTensor::UPtr ffnDown;

    ggml::JobGgmlTensor::UPtr ffnGateBias;
    ggml::JobGgmlTensor::UPtr ffnUpBias;
    ggml::JobGgmlTensor::UPtr ffnDownBias;

    ggml::JobGgmlTensor::UPtr postFfnNorm;

    ggml::JobGgmlTensor::UPtr ffnGateInp;
    ggml::JobGgmlTensor::UPtr ffnGateExps;
    ggml::JobGgmlTensor::UPtr ffnUpExps;
    ggml::JobGgmlTensor::UPtr ffnDownExps;
};

class JOBMODEL_EXPORT ModelWeights
{
public:
    ModelWeights() = default;
    ~ModelWeights() = default;

    ModelWeights(const ModelWeights &) = delete;
    ModelWeights &operator=(const ModelWeights &) = delete;
    ModelWeights(ModelWeights &&) noexcept = default;
    ModelWeights &operator=(ModelWeights &&) noexcept = default;

    [[nodiscard]] bool loadFromContext(ggml::JobGgmlContext &context, const ModelConfig &config);

    void clear() noexcept;

    [[nodiscard]] bool isLoaded() const noexcept
    {
        return m_tokenEmbd && m_tokenEmbd->isValid() &&
               m_outputNorm && m_outputNorm->isValid() &&
               !m_layers.empty();
    }

    [[nodiscard]] const ggml::JobGgmlTensor *tokenEmbd() const noexcept { return m_tokenEmbd.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor *outputNorm() const noexcept { return m_outputNorm.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor *outputNormBias() const noexcept { return m_outputNormBias.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor *output() const noexcept { return m_output ? m_output.get() : m_tokenEmbd.get(); }
    [[nodiscard]] bool hasTiedEmbedding() const noexcept { return m_output == nullptr; }

    [[nodiscard]] std::size_t layerCount() const noexcept { return m_layers.size(); }
    [[nodiscard]] const LayerWeights &layer(std::size_t index) const;
    [[nodiscard]] const std::vector<LayerWeights> &layers() const noexcept { return m_layers; }

    [[nodiscard]] const ggml::JobGgmlTensor *positionEmbd() const noexcept { return m_positionEmbd.get(); }
    [[nodiscard]] const ggml::JobGgmlTensor *typeEmbd() const noexcept { return m_typeEmbd.get(); }

private:
    [[nodiscard]] bool fail(std::string_view message);

    [[nodiscard]] ggml::JobGgmlTensor::UPtr findTensor(
        ggml::JobGgmlContext &context,
        std::string_view canonicalName,
        std::initializer_list<std::string_view> fallbackAliases = {}) const;

    ggml::JobGgmlTensor::UPtr m_tokenEmbd;
    ggml::JobGgmlTensor::UPtr m_outputNorm;
    ggml::JobGgmlTensor::UPtr m_outputNormBias;
    ggml::JobGgmlTensor::UPtr m_output;

    ggml::JobGgmlTensor::UPtr m_positionEmbd;
    ggml::JobGgmlTensor::UPtr m_typeEmbd;

    std::vector<LayerWeights> m_layers;
};

} // namespace job::model