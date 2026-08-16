#include "model_config.h"

namespace job::model {

ModelConfig::ModelConfig() = default;

bool ModelConfig::isValid() const noexcept
{
    if (!m_archConfig.isValid() ||
        !m_transformerConfig.isValid() ||
        !m_attentionConfig.isValid() ||
        !m_normConfig.isValid() ||
        !m_ropeConfig.isValid() ||
        !m_feedForwardConfig.isValid() ||
        !m_outputHeadConfig.isValid() ||
        !m_samplerConfig.isValid())
    {
        return false;
    }

    return !m_moeConfig.has_value() || m_moeConfig->isValid();
}

std::string_view ModelConfig::architectureName() const noexcept
{
    return m_archConfig.architectureName();
}

} // namespace job::model