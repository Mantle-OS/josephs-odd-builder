#pragma once

#include <cstdint>
#include <memory>

#include "jobmodel_export.h"

namespace job::model {

// Feed-forward block dimensions.
// Starts with just feedForwardLength -- moved out of TransformerConfig
// now rather than parked, but genuinely unused until this class grows
// to also hold activation flavor + gating (currently ArchConfig::
// hiddenActivation, staying put until gating itself gets designed).
class JOBMODEL_EXPORT FeedForwardConfig
{
public:
    using Ptr  = std::shared_ptr<FeedForwardConfig>;
    using WPtr = std::weak_ptr<FeedForwardConfig>;
    using UPtr = std::unique_ptr<FeedForwardConfig>;

    FeedForwardConfig();
    ~FeedForwardConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<FeedForwardConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<FeedForwardConfig>(); }

    FeedForwardConfig(const FeedForwardConfig &) = default;
    FeedForwardConfig &operator=(const FeedForwardConfig &) = default;
    FeedForwardConfig(FeedForwardConfig &&) noexcept = default;
    FeedForwardConfig &operator=(FeedForwardConfig &&) noexcept = default;

    // FFN intermediate (hidden) dimension.
    [[nodiscard]] uint32_t feedForwardLength() const noexcept { return m_feedForwardLength; }
    void setFeedForwardLength(uint32_t value) noexcept { m_feedForwardLength = value; }

    [[nodiscard]] bool isValid() const noexcept;

private:
    uint32_t m_feedForwardLength{0};
};

} // namespace job::model