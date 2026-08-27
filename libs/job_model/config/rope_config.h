#pragma once

#include <cstdint>
#include <memory>

#include <job_base_obj.h>

#include "jobmodel_export.h"
namespace job::model {

// Rotary Positional Embedding (RoPE) parameters.
// Applied to Q/K during attention, but kept as its own class rather than
// folded into AttentionConfig -- you already named "rope" as a distinct
// graph-split piece from "attention" and "gqa" from the start.
class JOBMODEL_EXPORT RopeConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<RopeConfig>;
    using WPtr = std::weak_ptr<RopeConfig>;
    using UPtr = std::unique_ptr<RopeConfig>;

    RopeConfig();
    ~RopeConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<RopeConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<RopeConfig>(); }

    RopeConfig(const RopeConfig &) = default;
    RopeConfig &operator=(const RopeConfig &) = default;
    RopeConfig(RopeConfig &&) noexcept = default;
    RopeConfig &operator=(RopeConfig &&) noexcept = default;

    // Number of rotary dimensions. 0 means "derive from attention head dimension".
    [[nodiscard]] uint32_t ropeDimensionCount() const noexcept { return m_ropeDimensionCount; }
    void setRopeDimensionCount(uint32_t value) noexcept { m_ropeDimensionCount = value; }

    // Base frequency (theta) for the rotary embedding. Throws if value is
    // non-finite or <= 0.
    [[nodiscard]] float ropeFreqBase() const noexcept { return m_ropeFreqBase; }
    void setRopeFreqBase(float value);

    // Frequency scaling factor (e.g. for context-extension schemes). Throws
    // if value is non-finite or <= 0.
    [[nodiscard]] float ropeFreqScale() const noexcept { return m_ropeFreqScale; }
    void setRopeFreqScale(float value);

    [[nodiscard]] bool isValid() const noexcept;

private:
    uint32_t m_ropeDimensionCount{0};
    float    m_ropeFreqBase{10000.0f};
    float    m_ropeFreqScale{1.0f};
};

} // namespace job::model