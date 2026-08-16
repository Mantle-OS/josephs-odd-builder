#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT EnvelopeFilterEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<EnvelopeFilterEffect>;
    using WPtr = std::weak_ptr<EnvelopeFilterEffect>;
    using UPtr = std::unique_ptr<EnvelopeFilterEffect>;

    static Ptr createShared() { return std::make_shared<EnvelopeFilterEffect>(); }
    static UPtr createUnique() { return std::make_unique<EnvelopeFilterEffect>(); }

    enum class FilterMode {
        LowPass,
        BandPass,
        HighPass
    };

    explicit EnvelopeFilterEffect() :
        AudioEffect{"envelope_filter"}
    {
        updateCoefficients();
    }

    EnvelopeFilterEffect(const EnvelopeFilterEffect &) = default;
    EnvelopeFilterEffect(EnvelopeFilterEffect &&) noexcept = default;

    EnvelopeFilterEffect &operator=(const EnvelopeFilterEffect &) = default;
    EnvelopeFilterEffect &operator=(EnvelopeFilterEffect &&) noexcept = default;

    ~EnvelopeFilterEffect() override = default;

    [[nodiscard]] float sensitivity() const noexcept { return m_sensitivity; }
    [[nodiscard]] float resonance() const noexcept { return m_q; }
    [[nodiscard]] float baseFrequency() const noexcept { return m_baseFreq; }
    [[nodiscard]] float attack() const noexcept { return m_attackMs; }
    [[nodiscard]] float release() const noexcept { return m_releaseMs; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] FilterMode mode() const noexcept { return m_mode; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Sensitivity / Envelope depth [0.0f, 10.0f]
    void setSensitivity(float sens) noexcept
    {
        m_sensitivity = std::max(0.0f, sens);
    }

    // Resonance Q factor [0.5f (flat), 20.0f (screaming self-oscillation peak)]
    void setResonance(float q) noexcept
    {
        m_q = std::clamp(q, 0.5f, 20.0f);
    }

    // Rest/Minimum cutoff frequency in Hz [20.0 Hz, 4000.0 Hz]
    void setBaseFrequency(float baseFreqHz) noexcept
    {
        m_baseFreq = std::clamp(baseFreqHz, 20.0f, 4000.0f);
    }

    // Envelope attack time in milliseconds [0.1 ms, 100.0 ms]
    void setAttack(float attackMs) noexcept
    {
        m_attackMs = std::max(0.1f, attackMs);
        updateCoefficients();
    }

    // Envelope release time in milliseconds [5.0 ms, 1000.0 ms]
    void setRelease(float releaseMs) noexcept
    {
        m_releaseMs = std::max(5.0f, releaseMs);
        updateCoefficients();
    }

    // Dry / Wet Mix [0.0f dry, 1.0f wet]
    void setMix(float mix) noexcept
    {
        m_mix = std::clamp(mix, 0.0f, 1.0f);
    }

    void setMode(FilterMode mode) noexcept
    {
        m_mode = mode;
    }

    void setSampleRate(float sampleRate) noexcept
    {
        if (sampleRate > 0.0f) {
            m_sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    void reset() noexcept
    {
        m_envelope = 0.0f;
        m_s1 = 0.0f;
        m_s2 = 0.0f;
    }

    [[nodiscard]] float process(float input) override final
    {
        // Envelope Follower ballistics
        const float absInput = std::abs(input);
        if (absInput > m_envelope) {
            m_envelope = absInput + m_attackCoeff * (m_envelope - absInput);
        } else {
            m_envelope = absInput + m_releaseCoeff * (m_envelope - absInput);
        }

        // Modulate cutoff frequency (exponential sweep across octaves)
        const float octaves = 5.0f;
        const float sweep = std::clamp(m_envelope * m_sensitivity, 0.0f, 1.0f);
        const float targetFreq = m_baseFreq * std::pow(2.0f, sweep * octaves);
        const float clampedFreq = std::clamp(targetFreq, 20.0f, m_sampleRate * 0.45f);

        // Chamberlin State-Variable Filter (SVF)
        // f = 2 * sin(pi * freq / sampleRate), q = 1 / Q
        const float f = 2.0f * std::sin(std::numbers::pi_v<float> * clampedFreq / m_sampleRate);
        const float damping = 1.0f / m_q;

        const float hp = input - m_s1 * damping - m_s2;
        const float bp = f * hp + m_s1;
        const float lp = f * bp + m_s2;

        m_s1 = bp;
        m_s2 = lp;

        // Select mode output
        float filtered = lp;
        if (m_mode == FilterMode::BandPass) {
            filtered = bp;
        } else if (m_mode == FilterMode::HighPass) {
            filtered = hp;
        }

        // Dry / Wet Crossfade
        return (input * (1.0f - m_mix)) + (filtered * m_mix);
    }

private:
    void updateCoefficients() noexcept
    {
        m_attackCoeff  = std::exp(-1.0f / ((m_attackMs * 0.001f) * m_sampleRate));
        m_releaseCoeff = std::exp(-1.0f / ((m_releaseMs * 0.001f) * m_sampleRate));
    }

private:
    float m_sensitivity = 2.5f;     // Gain multiplier for envelope modulation
    float m_q           = 4.0f;     // Peak resonance (wah "quack")
    float m_baseFreq    = 300.0f;   // Hz
    float m_attackMs    = 5.0f;     // Fast transient attack
    float m_releaseMs   = 120.0f;   // Smooth natural release
    float m_mix         = 1.0f;     // 100% wet default
    FilterMode m_mode   = FilterMode::BandPass; // Bandpass gives classic Mu-Tron / Crybaby sound
    float m_sampleRate  = 48000.0f;

    float m_attackCoeff  = 0.0f;
    float m_releaseCoeff = 0.0f;
    float m_envelope     = 0.0f;

    // Filter states
    float m_s1 = 0.0f;
    float m_s2 = 0.0f;
};

} // namespace job::sound