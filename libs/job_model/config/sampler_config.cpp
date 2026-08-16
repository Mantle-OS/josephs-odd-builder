#include "sampler_config.h"

#include <real_type.h>

namespace job::model {

SamplerConfig::SamplerConfig() = default;

bool SamplerConfig::isValid() const noexcept
{
    return core::isSafeFinite(m_temperature) &&
           core::isSafeFinite(m_topP) &&
           core::isSafeFinite(m_minP) &&
           core::isSafeFinite(m_repeatPenalty) &&
           core::isSafeFinite(m_frequencyPenalty) &&
           core::isSafeFinite(m_presencePenalty) &&
           m_temperature >= 0.0f &&
           m_topK >= 0 &&
           m_topP >= 0.0f && m_topP <= 1.0f &&
           m_minP >= 0.0f && m_minP <= 1.0f &&
           m_repeatPenalty >= 1.0f;
}
} // namespace job::model
