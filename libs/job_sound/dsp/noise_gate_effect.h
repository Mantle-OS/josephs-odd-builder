#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT NoiseGateEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<NoiseGateEffect>;
    using WPtr = std::weak_ptr<NoiseGateEffect>;
    using UPtr = std::unique_ptr<NoiseGateEffect>;

    static Ptr createShared() { return std::make_shared<NoiseGateEffect>(); }
    static UPtr createUnique() { return std::make_unique<NoiseGateEffect>(); }

    explicit NoiseGateEffect() :
        AudioEffect{"noisegate"}
    {
        updateCoefficients();
    }

    NoiseGateEffect(const NoiseGateEffect &) = default;
    NoiseGateEffect(NoiseGateEffect &&) noexcept = default;

    NoiseGateEffect &operator=(const NoiseGateEffect &) = default;
    NoiseGateEffect &operator=(NoiseGateEffect &&) noexcept = default;

    ~NoiseGateEffect() override = default;

    [[nodiscard]] float threshold() const noexcept { return m_openThresholdDb; }
    [[nodiscard]] float hysteresis() const noexcept { return m_hysteresisDb; }
    [[nodiscard]] float attack() const noexcept { return m_attackMs; }
    [[nodiscard]] float hold() const noexcept { return m_holdMs; }
    [[nodiscard]] float release() const noexcept { return m_releaseMs; }
    [[nodiscard]] float reduction() const noexcept { return m_reductionDb; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Threshold in decibels to open the gate [-90.0 dB, 0.0 dB]
    void setThreshold(float thresholdDb) noexcept
    {
        m_openThresholdDb = std::clamp(thresholdDb, -90.0f, 0.0f);
    }

    // Hysteresis in dB [0.0 dB, 18.0 dB] (Close threshold = Threshold - Hysteresis)
    void setHysteresis(float hysteresisDb) noexcept
    {
        m_hysteresisDb = std::clamp(hysteresisDb, 0.0f, 18.0f);
    }

    // Attack time in milliseconds [0.1 ms, 50.0 ms]
    void setAttack(float attackMs) noexcept
    {
        m_attackMs = std::max(0.1f, attackMs);
        updateCoefficients();
    }

    // Hold duration in milliseconds [0.0 ms, 500.0 ms]
    void setHold(float holdMs) noexcept
    {
        m_holdMs = std::max(0.0f, holdMs);
    }

    // Release time in milliseconds [1.0 ms, 1000.0 ms]
    void setRelease(float releaseMs) noexcept
    {
        m_releaseMs = std::max(1.0f, releaseMs);
        updateCoefficients();
    }

    // Maximum gain attenuation when closed in decibels [-96.0 dB, 0.0 dB]
    void setReduction(float reductionDb) noexcept
    {
        m_reductionDb = std::clamp(reductionDb, -96.0f, 0.0f);
        m_floorGain = std::pow(10.0f, m_reductionDb / 20.0f);
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
        m_currentGain = 1.0f;
        m_holdSamplesLeft = 0;
        m_gateOpen = true;
    }

    [[nodiscard]] float process(float input) override final
    {
        // Peak level detector in dB
        const float absInput = std::abs(input);
        const float inputDb = (absInput > 1e-5f) ? (20.0f * std::log10(absInput)) : -96.0f;

        const float closeThresholdDb = m_openThresholdDb - m_hysteresisDb;

        // State machine with hysteresis & hold timer
        if (inputDb >= m_openThresholdDb) {
            m_gateOpen = true;
            m_holdSamplesLeft = static_cast<std::size_t>((m_holdMs * 0.001f) * m_sampleRate);
        } else if (inputDb < closeThresholdDb) {
            if (m_holdSamplesLeft > 0) {
                --m_holdSamplesLeft;
            } else {
                m_gateOpen = false;
            }
        }

        // Target gain: 1.0 (open) or floorGain (attenuated/muted)
        const float targetGain = m_gateOpen ? 1.0f : m_floorGain;

        // Smooth ballistics
        if (targetGain > m_currentGain) {
            // Opening (Attack)
            m_currentGain = targetGain + m_attackCoeff * (m_currentGain - targetGain);
        } else {
            // Closing (Release)
            m_currentGain = targetGain + m_releaseCoeff * (m_currentGain - targetGain);
        }

        return input * m_currentGain;
    }

private:
    void updateCoefficients() noexcept
    {
        m_attackCoeff  = std::exp(-1.0f / ((m_attackMs * 0.001f) * m_sampleRate));
        m_releaseCoeff = std::exp(-1.0f / ((m_releaseMs * 0.001f) * m_sampleRate));
    }

private:
    float m_openThresholdDb = -45.0f;   // dB
    float m_hysteresisDb    = 6.0f;     // Close threshold = -51.0 dB
    float m_attackMs        = 1.5f;     // Fast attack (1.5ms)
    float m_holdMs          = 25.0f;    // 25ms hold
    float m_releaseMs       = 100.0f;   // 100ms release
    float m_reductionDb     = -96.0f;   // Total mute when closed
    float m_floorGain       = 0.0f;
    float m_sampleRate      = 48000.0f;

    float m_attackCoeff     = 0.0f;
    float m_releaseCoeff    = 0.0f;
    float m_currentGain     = 1.0f;
    std::size_t m_holdSamplesLeft = 0;
    bool  m_gateOpen        = true;
};

} // namespace job::sound