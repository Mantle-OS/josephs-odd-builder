#include "moe_config.h"

namespace job::model {

MoeConfig::MoeConfig() = default;

bool MoeConfig::isValid() const noexcept
{
    // Presence of a MoeConfig at all already means "this is a MoE model"
    // (see ModelConfig::moeConfig()); once here, the routing numbers still
    // have to actually make sense before unleashing them on the scheduler.
    return m_expertCount > 0 &&
           m_expertUsedCount > 0 &&
           m_expertUsedCount <= m_expertCount;
}

} // namespace job::model