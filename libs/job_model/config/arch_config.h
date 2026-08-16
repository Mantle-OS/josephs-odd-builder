#pragma once

#include <cstdint>
#include <string>

#include "model_architecture.h"
#include "jobmodel_export.h"

namespace job::model {

// Architecture-specific quirks and specialized knobs.
// Because every model family thinks standard transformer math is just a suggestion.
struct JOBMODEL_EXPORT ArchConfig {

    ModelArchitecture m_arch{ModelArchitecture::Unknown};
    std::string       m_archName{"unknown"};
    std::string       m_modelName{"unknown"};
    [[nodiscard]] std::string_view architectureName() const noexcept
    {
        return m_archName;
    }


    // Mixture of Experts (MoE) parameters - for when regular dense networks
    // just aren't expensive enough to train or run.
    uint32_t    m_expertCount{0};
    uint32_t    m_expertUsedCount{0};

    // Activation function flavor (e.g., "silu", "gelu", "relu")
    std::string m_hiddenActivation{"silu"};

    // Soft-capping and windowing (Gemma 2 / Qwen style controls)
    float       m_finalLogitSoftCapping{0.0f};
    float       m_attnLogitSoftCapping{0.0f};
    uint32_t    m_slidingWindowSize{0};

    // Architectural flags
    bool        m_attentionBias{false};
    bool        m_tieWordEmbeddings{false};

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // If it's a Mixture of Experts model, make sure the routing numbers
        // actually make sense before unleashing it on the scheduler.
        if (m_expertCount > 0 &&
            m_arch != ModelArchitecture::Unknown &&
            !m_archName.empty() && m_archName != "unknown" &&
            !m_modelName.empty() &&  m_modelName != "unknown")
        {
            return m_expertUsedCount > 0 && m_expertUsedCount <= m_expertCount;
        }

        return true;
    }
};

} // namespace job::model