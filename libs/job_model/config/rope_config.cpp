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
    return core::isSafeFinite(m_ropeFreqBase)  && m_ropeFreqBase > 0.0f &&
           core::isSafeFinite(m_ropeFreqScale) && m_ropeFreqScale > 0.0f;
}

} // namespace job::model