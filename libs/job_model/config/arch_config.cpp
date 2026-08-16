#include "arch_config.h"

namespace job::model {

ArchConfig::ArchConfig() = default;

bool ArchConfig::isValid() const noexcept
{
    return m_arch != ModelArchitecture::Unknown &&
           !m_archName.empty() && m_archName != "unknown" &&
           !m_modelName.empty() && m_modelName != "unknown";
}

} // namespace job::model
