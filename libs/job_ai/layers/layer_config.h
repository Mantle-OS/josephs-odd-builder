#pragma once

#include <cstdint>
#include <string_view>
#include <array>
#include <cstddef>

#include "activation_types.h"
#include "adapter_types.h"
#include "layer_types.h"

namespace job::ai::layers {
struct LayerConfig {
    // serializable
    std::array<char, 16>    name{"Layer"};                                  // A simple string view for debug
    // 32-bit group (no padding inside this block)
    uint32_t                inputs{0};                                      // In-features
    uint32_t                outputs{0};                                     // Out-features
    uint32_t                maxSequence{0};                                 // For pre-allocating buffers (Attention)
    uint32_t                numHeads{8};                                    // the number od XXXXXXXXX
    uint32_t                numExperts{0};                                  // Number of experts for the MOE
    uint32_t                topK{2};                                        // Active experts per token
    // 8-bit group
    LayerType               type{LayerType::Dense};                         // The Type of Layer that this is
    LayerMode               mode{LayerMode::Training};                      // The mode that this layer is currently in
    adapters::AdapterType   adapterType{adapters::AdapterType::Flash};      // The "Engine" (Flash, FMM, etc.)
    comp::ActivationType    activation{comp::ActivationType::Swish};        // Default speed demon
    // flags
    uint8_t flags{0};

    // X of 4 "likely" 48B total.... I think silly math
    uint8_t _pad[3]{};

    constexpr bool causal() const noexcept
    {
        return flags & LayerFlags::Causal;
    }

    constexpr bool hasRouter() const noexcept
    {
        return flags & LayerFlags::HasRouter;
    }

    constexpr bool hasBias() const noexcept
    {
        return flags & LayerFlags::HasBias;
    }

    constexpr void setCausal(bool flag) noexcept
    {
        flag ? flags |= LayerFlags::Causal : flags &= ~LayerFlags::Causal;
    }

    constexpr void setHasRouter(bool flag) noexcept
    {
        flag ? flags |= LayerFlags::HasRouter : flags &= ~LayerFlags::HasRouter;
    }

    constexpr void setHasBias(bool flag) noexcept
    {
        flag ? flags |= LayerFlags::HasBias   : flags &= ~LayerFlags::HasBias;
    }

    constexpr void setName(std::string_view str_v) noexcept
    {
        for (auto &c : name)
            c = '\0';

        const std::size_t n = (str_v.size() < name.size() - 1) ?
                                  str_v.size() :
                                  (name.size() - 1);
        for (std::size_t i = 0; i < n; ++i)
            name[i] = str_v[i];
    }
};


namespace LayerPresets{

[[nodiscard]] inline LayerConfig DenseConfig(
    std::string_view name       = "Dense",
    bool bias                   = true,
    comp::ActivationType act    = comp::ActivationType::Swish
    )
{
    LayerConfig cfg;
    cfg.type = LayerType::Dense;
    cfg.setName(name);
    cfg.setHasBias(bias);
    cfg.activation = act;
    return cfg;
}


[[nodiscard]] inline LayerConfig AttentionConfig(
    std::string_view    name    = "Attention",
    uint32_t            heads   = 8,
    bool                causal  = true
    )
{
    LayerConfig cfg;
    cfg.type = LayerType::Attention;
    cfg.setName(name);
    cfg.adapterType = adapters::AdapterType::Flash;
    cfg.setHasRouter(false);
    cfg.numHeads = heads;
    cfg.setCausal(causal);
    cfg.setHasBias(false);
    return cfg;
}

[[nodiscard]] inline LayerConfig MoEConfig(
    uint32_t experts = 8,
    uint32_t topK = 2,
    std::string_view name = "GenericMoeLayer"
    )
{
    LayerConfig cfg;
    cfg.type = LayerType::SparseMoE;
    cfg.setName(name);
    cfg.setHasRouter(true);
    cfg.numExperts = experts;
    cfg.topK = topK;
    return cfg;
}


} // LayerPresets

} // namespace job::ai::layers
