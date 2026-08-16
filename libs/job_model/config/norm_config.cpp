#include "norm_config.h"

#include <stdexcept>

#include <real_type.h>

namespace job::model {

NormConfig::NormConfig() = default;

void NormConfig::setRmsNormEps(float value)
{
    if (!core::isSafeFinite(value) || value <= 0.0f) {
        throw std::invalid_argument{
            "NormConfig rmsNormEps must be finite and greater than zero"
        };
    }

    m_rmsNormEps = value;
}

void NormConfig::setLayerNormEps(float value)
{
    if (!core::isSafeFinite(value) || value <= 0.0f) {
        throw std::invalid_argument{
            "NormConfig layerNormEps must be finite and greater than zero"
        };
    }

    m_layerNormEps = value;
}

bool NormConfig::isValid() const noexcept
{
    return core::isSafeFinite(m_rmsNormEps)   && m_rmsNormEps > 0.0f &&
           core::isSafeFinite(m_layerNormEps) && m_layerNormEps > 0.0f;
}

} // namespace job::model