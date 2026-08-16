#pragma once

#include <cstdint>
#include <memory>

#include "jobmodel_export.h"

namespace job::model {

// Mixture of Experts (MoE) routing parameters.
// Lives on its own -- and ModelConfig holds it as std::optional<MoeConfig> --
// because most architectures (Llama, Gemma, Phi...) have no concept of an
// expert at all. A dense model shouldn't carry expert bookkeeping fields
// it will never use, and (eventually) a QML-facing ModelConfig shouldn't
// expose MoE properties on model instances that aren't MoE.
class JOBMODEL_EXPORT MoeConfig
{
public:
    using Ptr  = std::shared_ptr<MoeConfig>;
    using WPtr = std::weak_ptr<MoeConfig>;
    using UPtr = std::unique_ptr<MoeConfig>;

    MoeConfig();
    ~MoeConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<MoeConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<MoeConfig>(); }

    MoeConfig(const MoeConfig &) = default;
    MoeConfig &operator=(const MoeConfig &) = default;
    MoeConfig(MoeConfig &&) noexcept = default;
    MoeConfig &operator=(MoeConfig &&) noexcept = default;

    // Total number of experts available in the router.
    [[nodiscard]] uint32_t expertCount() const noexcept { return m_expertCount; }
    void setExpertCount(uint32_t count) noexcept { m_expertCount = count; }

    // Number of experts activated per token.
    [[nodiscard]] uint32_t expertUsedCount() const noexcept { return m_expertUsedCount; }
    void setExpertUsedCount(uint32_t count) noexcept { m_expertUsedCount = count; }

    [[nodiscard]] bool isValid() const noexcept;

private:
    uint32_t m_expertCount{0};
    uint32_t m_expertUsedCount{0};
};

} // namespace job::model