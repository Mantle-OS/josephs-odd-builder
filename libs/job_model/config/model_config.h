#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <job_base_obj.h>

#include "arch_config.h"
#include "attention_config.h"
#include "feed_forward_config.h"
#include "moe_config.h"
#include "norm_config.h"
#include "output_head_config.h"
#include "rope_config.h"
#include "transformer_config.h"
#include "sampler_config.h"
#include "config/device_config.h"
#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT ModelConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<ModelConfig>;
    using WPtr = std::weak_ptr<ModelConfig>;
    using UPtr = std::unique_ptr<ModelConfig>;

    ModelConfig();
    ~ModelConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<ModelConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<ModelConfig>(); }

    ModelConfig(const ModelConfig &) = default;
    ModelConfig &operator=(const ModelConfig &) = default;
    ModelConfig(ModelConfig &&) noexcept = default;
    ModelConfig &operator=(ModelConfig &&) noexcept = default;

    // Mutable accessors: readers (GGUF / HF JSON / SafeTensors) populate
    // these sub-configs field-by-field in place rather than reaching past
    // ModelConfig's own encapsulation.
    [[nodiscard]] ArchConfig &archConfig() noexcept { return m_archConfig; }
    [[nodiscard]] const ArchConfig &archConfig() const noexcept { return m_archConfig; }
    void setArchConfig(ArchConfig config) noexcept { m_archConfig = std::move(config); }

    [[nodiscard]] TransformerConfig &transformerConfig() noexcept { return m_transformerConfig; }
    [[nodiscard]] const TransformerConfig &transformerConfig() const noexcept { return m_transformerConfig; }
    void setTransformerConfig(TransformerConfig config) noexcept { m_transformerConfig = std::move(config); }

    // Always present, unlike MoeConfig below: every architecture this
    // library targets has attention, normalization, RoPE, and an FFN
    // block -- there's no "absent means N/A" case the way MoE has.
    [[nodiscard]] AttentionConfig &attentionConfig() noexcept { return m_attentionConfig; }
    [[nodiscard]] const AttentionConfig &attentionConfig() const noexcept { return m_attentionConfig; }
    void setAttentionConfig(AttentionConfig config) noexcept { m_attentionConfig = std::move(config); }

    [[nodiscard]] NormConfig &normConfig() noexcept { return m_normConfig; }
    [[nodiscard]] const NormConfig &normConfig() const noexcept { return m_normConfig; }
    void setNormConfig(NormConfig config) noexcept { m_normConfig = std::move(config); }

    [[nodiscard]] RopeConfig &ropeConfig() noexcept { return m_ropeConfig; }
    [[nodiscard]] const RopeConfig &ropeConfig() const noexcept { return m_ropeConfig; }
    void setRopeConfig(RopeConfig config) noexcept { m_ropeConfig = std::move(config); }

    [[nodiscard]] FeedForwardConfig &feedForwardConfig() noexcept { return m_feedForwardConfig; }
    [[nodiscard]] const FeedForwardConfig &feedForwardConfig() const noexcept { return m_feedForwardConfig; }
    void setFeedForwardConfig(FeedForwardConfig config) noexcept { m_feedForwardConfig = std::move(config); }

    [[nodiscard]] OutputHeadConfig &outputHeadConfig() noexcept { return m_outputHeadConfig; }
    [[nodiscard]] const OutputHeadConfig &outputHeadConfig() const noexcept { return m_outputHeadConfig; }
    void setOutputHeadConfig(OutputHeadConfig config) noexcept { m_outputHeadConfig = std::move(config); }

    [[nodiscard]] SamplerConfig &samplerConfig() noexcept { return m_samplerConfig; }
    [[nodiscard]] const SamplerConfig &samplerConfig() const noexcept { return m_samplerConfig; }
    void setSamplerConfig(SamplerConfig config) noexcept { m_samplerConfig = std::move(config); }

    [[nodiscard]] DeviceConfig &deviceConfig() noexcept { return m_deviceConfig; }
    [[nodiscard]] const DeviceConfig &deviceConfig() const noexcept { return m_deviceConfig; }
    void setDeviceConfig(DeviceConfig config) noexcept { m_deviceConfig = std::move(config); }


    // Mixture of Experts (MoE) routing parameters. Lives at the ModelConfig
    [[nodiscard]] bool hasMoeConfig() const noexcept { return m_moeConfig.has_value(); }
    [[nodiscard]] MoeConfig &moeConfig()
    {
        if (!m_moeConfig.has_value())
            m_moeConfig.emplace();
        return *m_moeConfig;
    }
    [[nodiscard]] const MoeConfig *moeConfig() const noexcept
    {
        return m_moeConfig.has_value() ? &(*m_moeConfig) : nullptr;
    }
    void setMoeConfig(MoeConfig config) { m_moeConfig = std::move(config); }
    void clearMoeConfig() noexcept { m_moeConfig.reset(); }

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] std::string_view architectureName() const noexcept;

private:
    ArchConfig                  m_archConfig;
    TransformerConfig           m_transformerConfig;
    AttentionConfig             m_attentionConfig;
    NormConfig                  m_normConfig;
    RopeConfig                  m_ropeConfig;
    FeedForwardConfig           m_feedForwardConfig;
    OutputHeadConfig            m_outputHeadConfig;
    SamplerConfig               m_samplerConfig;
    DeviceConfig                m_deviceConfig;
    std::optional<MoeConfig>    m_moeConfig;
};

} // namespace job::model