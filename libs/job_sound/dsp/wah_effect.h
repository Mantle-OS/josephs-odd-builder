#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT WahEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<WahEffect>;
    using WPtr = std::weak_ptr<WahEffect>;
    using UPtr = std::unique_ptr<WahEffect>;

    static Ptr createShared() { return std::make_shared<WahEffect>(); }
    static UPtr createUnique() { return std::make_unique<WahEffect>(); }

    explicit WahEffect() :
        AudioEffect{"wah"}
    {
        updateFilter();
    }

    WahEffect(const WahEffect &) = default;
    WahEffect(WahEffect &&) noexcept = default;

    WahEffect &operator=(const WahEffect &) = default;
    WahEffect &operator=(WahEffect &&) noexcept = default;

    ~WahEffect() override = default;

    [[nodiscard]] float pedalPosition() const noexcept { return m_pedalPos; }
    [[nodiscard]] float minFrequency() const noexcept { return m_minFreq; }
    [[nodiscard]] float maxFrequency() const noexcept { return m_maxFreq; }
    [[nodiscard]] float resonance() const noexcept { return m_q; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Pedal position [0.0f (Heel / Bass), 1.0f (Toe / Treble)]
    void setPedalPosition(float position) noexcept
    {
        const float clamped = std::clamp(position, 0.0f, 1.0f);
        if (clamped != m_pedalPos) {
            m_pedalPos = clamped;
            updateFilter();
        }
    }

    // Heel frequency in Hz [100.0 Hz, 800.0 Hz]
    void setMinFrequency(float freqHz) noexcept
    {
        m_minFreq = std::clamp(freqHz, 100.0f, 800.0f);
        updateFilter();
    }

    // Toe frequency in Hz [1000.0 Hz, 5000.0 Hz]
    void setMaxFrequency(float freqHz) noexcept
    {
        m_maxFreq = std::clamp(freqHz, 1000.0f, 5000.0f);
        updateFilter();
    }

    // Center resonance Q [1.0f, 15.0f]
    void setResonance(float q) noexcept
    {
        m_q = std::clamp(q, 1.0f, 15.0f);
        updateFilter();
    }

    // Dry / Wet Mix [0.0f dry, 1.0f wet]
    void setMix(float mix) noexcept
    {
        m_mix = std::clamp(mix, 0.0f, 1.0f);
    }

    void setSampleRate(float sampleRate) noexcept
    {
        if (sampleRate > 0.0f && sampleRate != m_sampleRate) {
            m_sampleRate = sampleRate;
            updateFilter();
        }
    }

    void reset() noexcept
    {
        m_s1 = 0.0f;
        m_s2 = 0.0f;
    }

    [[nodiscard]] float process(float input) override final
    {
        // Direct Form II Transposed Biquad processing
        // y[n] = b0 * x[n] + s1
        // s1   = b1 * x[n] - a1 * y[n] + s2
        // s2   = b2 * x[n] - a2 * y[n]
        const float filtered = (m_b0 * input) + m_s1;
        m_s1 = (m_b1 * input) - (m_a1 * filtered) + m_s2;
        m_s2 = (m_b2 * input) - (m_a2 * filtered);

        return (input * (1.0f - m_mix)) + (filtered * m_mix);
    }

private:
    void updateFilter() noexcept
    {
        // Exponential taper: minFreq * (maxFreq / minFreq)^pedalPos
        const float centerFreq = m_minFreq * std::pow(m_maxFreq / m_minFreq, m_pedalPos);
        const float clampedFreq = std::clamp(centerFreq, 20.0f, m_sampleRate * 0.45f);

        // Crybaby inductor dynamic Q scaling: higher Q at high frequencies
        const float dynamicQ = m_q * (1.0f + 0.5f * m_pedalPos);

        // Standard peaking bandpass biquad (constant 0 dB peak gain)
        const float w0 = 2.0f * std::numbers::pi_v<float> * clampedFreq / m_sampleRate;
        const float alpha = std::sin(w0) / (2.0f * dynamicQ);
        const float cosW0 = std::cos(w0);

        const float a0 = 1.0f + alpha;
        const float invA0 = 1.0f / a0;

        m_b0 = alpha * invA0;
        m_b1 = 0.0f;
        m_b2 = -alpha * invA0;
        m_a1 = (-2.0f * cosW0) * invA0;
        m_a2 = (1.0f - alpha) * invA0;
    }

private:
    float m_pedalPos   = 0.5f;     // Mid position
    float m_minFreq    = 350.0f;   // Hz (Heel down)
    float m_maxFreq    = 2200.0f;  // Hz (Toe down)
    float m_q          = 4.5f;     // Wah vocal formant resonance
    float m_mix        = 1.0f;     // 100% wet default
    float m_sampleRate = 48000.0f;

    // Normalized biquad filter coefficients
    float m_b0 = 0.0f;
    float m_b1 = 0.0f;
    float m_b2 = 0.0f;
    float m_a1 = 0.0f;
    float m_a2 = 0.0f;

    // Filter states
    float m_s1 = 0.0f;
    float m_s2 = 0.0f;
};

} // namespace job::sound