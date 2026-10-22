#include "rope_config.h"

#include <stdexcept>

#include <real_type.h>

namespace job::model {

RopeConfig::RopeConfig() = default;

void RopeConfig::setRopeFreqBase(float value)
{
    if (!core::isSafeFinite(value) || value <= 0.0f)
        throw std::invalid_argument{ "RopeConfig ropeFreqBase must be finite and greater than zero" };

    m_ropeFreqBase = value;
}

void RopeConfig::setRopeFreqScale(float value)
{
    if (!core::isSafeFinite(value) || value <= 0.0f)
        throw std::invalid_argument{ "RopeConfig ropeFreqScale must be finite and greater than zero" };

    m_ropeFreqScale = value;
}

bool RopeConfig::isValid() const noexcept
{
    if(m_type == RopeType::Normal || m_type == RopeType::NormalInplace){
        return core::isSafeFinite(m_ropeFreqBase)  && m_ropeFreqBase > 0.0f && core::isSafeFinite(m_ropeFreqScale) && m_ropeFreqScale > 0.0f;
    }else{
        const bool multi = ropeType() == RopeType::Multi || ropeType() == RopeType::MultiInplace || ropeType() == RopeType::MultiBack;
        if (multi && m_sections.size() != 4)
            return false;

        return core::isSafeFinite(m_ropeFreqBase)  && m_ropeFreqBase > 0.0f &&
               core::isSafeFinite(m_ropeFreqScale) && m_ropeFreqScale > 0.0f &&
               core::isSafeFinite(m_extFactor) &&
               core::isSafeFinite(m_attnFactor) &&
               core::isSafeFinite(m_betaFast) &&
               core::isSafeFinite(m_betaSlow);
    }
}

} // namespace job::model