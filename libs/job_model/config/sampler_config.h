#pragma once

#include <cstdint>

#include "jobmodel_export.h"

namespace job::model {

// The control panel for token generation.
// Because without these knobs, your model either loops the same phrase
// into infinity or hallucinates poetry about toaster manuals.
struct JOBMODEL_EXPORT SamplerConfig {
    float    m_temperature{0.8f};
    int32_t  m_topK{40};
    float    m_topP{0.95f};
    float    m_minP{0.05f};
    float    m_repeatPenalty{1.1f};
    float    m_frequencyPenalty{0.0f};
    float    m_presencePenalty{0.0f};
    uint64_t m_seed{1337};
    bool     m_greedy{false};

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // Negative temperature or probability out of bounds
        // will break math faster than division by zero.
        return m_temperature >= 0.0f &&
               m_topP >= 0.0f && m_topP <= 1.0f &&
               m_minP >= 0.0f && m_minP <= 1.0f &&
               m_repeatPenalty >= 1.0f;
    }
};

} // namespace job::model