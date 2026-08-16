#pragma once

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT FlangerEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<FlangerEffect>;
    using WPtr = std::weak_ptr<FlangerEffect>;
    using UPtr = std::unique_ptr<FlangerEffect>;

    static Ptr createShared() { return std::make_shared<FlangerEffect>(); }
    static UPtr createUnique() { return std::make_unique<FlangerEffect>(); }

    explicit FlangerEffect()
        : AudioEffect{"flanger"},
        m_phase(0.0f),
        m_maxDelay(1024)
    {
        m_buffer.resize(m_maxDelay);
    }

    FlangerEffect(const FlangerEffect &) = default;
    FlangerEffect(FlangerEffect &&) noexcept = default;

    FlangerEffect &operator=(const FlangerEffect &) = default;
    FlangerEffect &operator=(FlangerEffect &&) noexcept = default;

    ~FlangerEffect() override = default;

    [[nodiscard]] float depth() const noexcept { return m_depth; }
    [[nodiscard]] float rate() const noexcept { return m_rate; }
    [[nodiscard]] float feedback() const noexcept { return m_feedback; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    void setDepth(float depth) noexcept { m_depth = depth; }
    void setRate(float rate) noexcept { m_rate = rate; }
    void setFeedback(float feedback) noexcept { m_feedback = feedback; }
    void setMix(float mix) noexcept { m_mix = mix; }
    void setSampleRate(float sampleRate) noexcept { m_sampleRate = sampleRate; }


    // [[FIXME]] One thing worth keeping in mind for later: m_depth * m_sampleRate must remain below m_maxDelay,
    // otherwise the delay index math can wrap to somewhere other than the intended delay position.
    [[nodiscard]] float process(float input) override final
    {
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
        const float delaySamples = (m_depth * m_sampleRate / 2.0f) * (1.0f + std::sin(m_phase));
        m_phase += twoPi * m_rate / m_sampleRate;

        if (m_phase > twoPi)
            m_phase -= twoPi;

        int index = m_writeIndex - static_cast<int>(delaySamples);
        if (index < 0)
            index += m_maxDelay;

        const float delayed = m_buffer.at(index % m_maxDelay);
        const float output = input * (1.0f - m_mix) + delayed * m_mix;
        m_buffer[m_writeIndex % m_maxDelay] = input + delayed * m_feedback;

        m_writeIndex = (m_writeIndex + 1) % m_maxDelay;
        return output;
    }

private:
    float m_depth      = 0.002f;   // seconds
    float m_rate       = 0.25f;    // Hz
    float m_feedback   = 0.3f;
    float m_mix        = 0.5f;
    float m_sampleRate = 48000.0f;

    std::vector<float> m_buffer;

    int   m_writeIndex = 0;
    float m_phase      = 0.0f;
    int   m_maxDelay   = 1024;
};

} // namespace job::sound