#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT DelayEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<DelayEffect>;
    using WPtr = std::weak_ptr<DelayEffect>;
    using UPtr = std::unique_ptr<DelayEffect>;

    static Ptr createShared() { return std::make_shared<DelayEffect>(); }
    static UPtr createUnique() { return std::make_unique<DelayEffect>(); }

    explicit DelayEffect() :
        AudioEffect{"delay"},
        m_maxDelay(96000) // 2.0s @ 48kHz
    {
        m_buffer.resize(m_maxDelay, 0.0f);
    }

    DelayEffect(const DelayEffect &) = default;
    DelayEffect(DelayEffect &&) noexcept = default;

    DelayEffect &operator=(const DelayEffect &) = default;
    DelayEffect &operator=(DelayEffect &&) noexcept = default;

    ~DelayEffect() override = default;

    [[nodiscard]] float time() const noexcept { return m_time; }
    [[nodiscard]] float feedback() const noexcept { return m_feedback; }
    [[nodiscard]] float damping() const noexcept { return m_damping; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    void setTime(float timeSeconds) noexcept { m_time = std::clamp(timeSeconds, 0.0f, static_cast<float>(m_maxDelay) / m_sampleRate); }
    void setFeedback(float feedback) noexcept { m_feedback = std::clamp(feedback, 0.0f, 0.98f); }
    void setDamping(float damping) noexcept { m_damping = std::clamp(damping, 0.0f, 1.0f); }
    void setMix(float mix) noexcept { m_mix = std::clamp(mix, 0.0f, 1.0f); }

    void setSampleRate(float sampleRate) noexcept
    {
        if (sampleRate > 0.0f)
            m_sampleRate = sampleRate;
    }

    void reset() noexcept
    {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writeIndex = 0;
        m_filterState = 0.0f;
    }

    [[nodiscard]] float process(float input) override final
    {
        const float delaySamples = std::clamp(m_time * m_sampleRate, 0.0f, static_cast<float>(m_maxDelay - 1));

        int index = m_writeIndex - static_cast<int>(delaySamples);
        if (index < 0)
            index += m_maxDelay;

        const float delayed = m_buffer.at(index % m_maxDelay);

        // One-pole low-pass filter on feedback for warm tape decay
        m_filterState = (m_damping * delayed) + ((1.0f - m_damping) * m_filterState);

        const float output = input * (1.0f - m_mix) + delayed * m_mix;

        m_buffer[m_writeIndex % m_maxDelay] = input + (m_filterState * m_feedback);
        m_writeIndex = (m_writeIndex + 1) % m_maxDelay;

        return output;
    }

private:
    float m_time        = 0.35f;   // seconds (350ms default)
    float m_feedback    = 0.4f;    // [0.0, 0.98]
    float m_damping     = 0.8f;    // [0.0 (dark), 1.0 (bright)]
    float m_mix         = 0.35f;   // [0.0 dry, 1.0 wet]
    float m_sampleRate  = 48000.0f;

    std::vector<float> m_buffer;

    int   m_writeIndex  = 0;
    int   m_maxDelay    = 96000;
    float m_filterState = 0.0f;
};

} // namespace job::sound