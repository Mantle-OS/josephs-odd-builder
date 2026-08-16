#include "output_head_config.h"

#include <stdexcept>

#include <real_type.h>

namespace job::model {

OutputHeadConfig::OutputHeadConfig() = default;

void OutputHeadConfig::setFinalLogitSoftCapping(float value)
{
    if (!core::isSafeFinite(value) || value < 0.0f) {
        throw std::invalid_argument{
            "OutputHeadConfig finalLogitSoftCapping must be finite and non-negative"
        };
    }

    m_finalLogitSoftCapping = value;
}

bool OutputHeadConfig::isValid() const noexcept
{
    // tieWordEmbeddings (bool) has no invalid representation on its own.
    return core::isSafeFinite(m_finalLogitSoftCapping) &&
           m_finalLogitSoftCapping >= 0.0f;
}

} // namespace job::model