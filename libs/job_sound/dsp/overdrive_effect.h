#pragma once

#include <cmath>
#include <memory>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT OverdriveEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<OverdriveEffect>;
    using WPtr = std::weak_ptr<OverdriveEffect>;
    using UPtr = std::unique_ptr<OverdriveEffect>;

    static Ptr createShared() { return std::make_shared<OverdriveEffect>(); }
    static UPtr createUnique() { return std::make_unique<OverdriveEffect>(); }

    explicit OverdriveEffect() : AudioEffect("overdrive")
    {
    }

    OverdriveEffect(const OverdriveEffect &) = default;
    OverdriveEffect(OverdriveEffect &&) noexcept = default;

    OverdriveEffect &operator=(const OverdriveEffect &) = default;
    OverdriveEffect &operator=(OverdriveEffect &&) noexcept = default;

    ~OverdriveEffect() override = default;

    [[nodiscard]] float gain() const noexcept { return m_gain; }
    [[nodiscard]] float level() const noexcept { return m_level; }

    void setGain(float gain) noexcept { m_gain = gain; }
    void setLevel(float level) noexcept { m_level = level; }

    [[nodiscard]] float process(float input) override final
    {
        const float boosted = input * m_gain;
        const float clipped = std::tanh(boosted);
        return clipped * m_level;
    }

private:
    float m_gain  = 2.0f; // Input gain
    float m_level = 1.0f; // Output level
};

} // namespace job::sound