#pragma once

#include <string_view>

#include "arch_config.h"
#include "transformer_config.h"
#include "sampler_config.h"
#include "jobmodel_export.h"

namespace job::model {

// The top-level composition node.
// It doesn't do the heavy math itself; it just introduces the transformer
// to the sampler so they can work together without a messy global state.
struct JOBMODEL_EXPORT ModelConfig {
    ArchConfig        m_archConfig;
    TransformerConfig m_transformerConfig;
    SamplerConfig     m_samplerConfig;

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // A model config is invalid if the architecture hasn't been identified.
        return m_archConfig.m_arch != ModelArchitecture::Unknown &&
               m_transformerConfig.isValid() &&
               m_samplerConfig.isValid() &&
               m_archConfig.isValid();
    }

    [[nodiscard]] std::string_view architectureName() const noexcept
    {
        return m_archConfig.architectureName();
    }
};

} // namespace job::model
