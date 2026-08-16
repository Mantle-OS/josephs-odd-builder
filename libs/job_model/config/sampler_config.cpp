#include "sampler_config.h"

namespace job::model {

SamplerConfig::SamplerConfig() = default;

bool SamplerConfig::isValid() const noexcept
{
    // Negative temperature or probability out of bounds
    // will break math faster than division by zero.
    return m_temperature >= 0.0f &&
           m_topP >= 0.0f && m_topP <= 1.0f &&
           m_minP >= 0.0f && m_minP <= 1.0f &&
           m_repeatPenalty >= 1.0f;
}

} // namespace job::model
