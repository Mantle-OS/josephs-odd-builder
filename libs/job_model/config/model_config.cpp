#include "model_config.h"

namespace job::model {

ModelConfig::ModelConfig() = default;

bool ModelConfig::isValid() const noexcept
{
    // A model config is invalid if the architecture hasn't been identified.
    if (m_archConfig.arch() == ModelArchitecture::Unknown ||
        !m_transformerConfig.isValid() ||
        !m_attentionConfig.isValid() ||
        !m_normConfig.isValid() ||
        !m_ropeConfig.isValid() ||
        !m_feedForwardConfig.isValid() ||
        !m_outputHeadConfig.isValid() ||
        !m_samplerConfig.isValid() ||
        !m_archConfig.isValid())
    {
        return false;
    }

    // If this instance carries MoE routing parameters, they have to
    // actually make sense before unleashing it on the scheduler.
    if (m_moeConfig.has_value())
        return m_moeConfig->isValid();

    return true;
}

std::string_view ModelConfig::architectureName() const noexcept
{
    return m_archConfig.architectureName();
}

} // namespace job::model