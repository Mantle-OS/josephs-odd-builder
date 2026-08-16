#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT CompressorEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<CompressorEffect>;
    using WPtr = std::weak_ptr<CompressorEffect>;
    using UPtr = std::unique_ptr<CompressorEffect>;

    static Ptr createShared() { return std::make_shared<CompressorEffect>(); }
    static UPtr createUnique() { return std::make_unique<CompressorEffect>(); }

    explicit CompressorEffect() :
        AudioEffect{"compressor"}
    {
        updateCoefficients();
    }

    CompressorEffect(const CompressorEffect &) = default;
    CompressorEffect(CompressorEffect &&) noexcept = default;

    CompressorEffect &operator=(const CompressorEffect &) = default;
    CompressorEffect &operator=(CompressorEffect &&) noexcept = default;

    ~CompressorEffect() override = default;

    [[nodiscard]] float threshold() const noexcept { return m_thresholdDb; }
    [[nodiscard]] float ratio() const noexcept { return m_ratio; }
    [[nodiscard]] float attack() const noexcept { return m_attackMs; }
    [[nodiscard]] float release() const noexcept { return m_releaseMs; }
    [[nodiscard]] float makeupGain() const noexcept { return m_makeupDb; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Threshold in decibels [-60.0 dB, 0.0 dB]
    void setThreshold(float thresholdDb) noexcept
    {
        m_thresholdDb = std::clamp(thresholdDb, -60.0f, 0.0f);
    }

    // Ratio [1.0 (no compression), 20.0+ (limiting)]
    void setRatio(float ratio) noexcept
    {
        m_ratio = std::max(1.0f, ratio);
    }

    // Attack time in milliseconds [0.1 ms, 500.0 ms]
    void setAttack(float attackMs) noexcept
    {
        m_attackMs = std::max(0.1f, attackMs);
        updateCoefficients();
    }

    // Release time in milliseconds [1.0 ms, 2000.0 ms]
    void setRelease(float releaseMs) noexcept
    {
        m_releaseMs = std::max(1.0f, releaseMs);
        updateCoefficients();
    }

    // Output makeup gain in decibels [0.0 dB, 36.0 dB]
    void setMakeupGain(float makeupDb) noexcept
    {
        m_makeupDb = std::clamp(makeupDb, 0.0f, 36.0f);
        m_makeupLinear = std::pow(10.0f, m_makeupDb / 20.0f);
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
        m_envelopeDb = -96.0f;
    }

    [[nodiscard]] float process(float input) override final
    {
        const float absInput = std::abs(input);
        const float inputDb = (absInput > 1e-5f) ? (20.0f * std::log10(absInput)) : -96.0f;
        float targetGainDb = 0.0f;
        if (inputDb > m_thresholdDb)
            targetGainDb = (m_thresholdDb + (inputDb - m_thresholdDb) / m_ratio) - inputDb;

        if (targetGainDb < m_envelopeDb)
            m_envelopeDb = targetGainDb + m_attackCoeff * (m_envelopeDb - targetGainDb);
        else
            m_envelopeDb = targetGainDb + m_releaseCoeff * (m_envelopeDb - targetGainDb);


        const float linearGain = std::pow(10.0f, m_envelopeDb / 20.0f) * m_makeupLinear;

        return input * linearGain;
    }

private:
    void updateCoefficients() noexcept
    {
        // One-pole exponential decay: coeff = exp(-1.0 / (timeInSeconds * sampleRate))
        m_attackCoeff = std::exp(-1.0f / ((m_attackMs * 0.001f) * m_sampleRate));
        m_releaseCoeff = std::exp(-1.0f / ((m_releaseMs * 0.001f) * m_sampleRate));
    }

private:
    float m_thresholdDb   = -12.0f;  // dB
    float m_ratio         = 4.0f;    // 4:1 compression
    float m_attackMs      = 10.0f;   // ms
    float m_releaseMs     = 100.0f;  // ms
    float m_makeupDb      = 0.0f;    // dB
    float m_makeupLinear  = 1.0f;
    float m_sampleRate    = 48000.0f;

    float m_envelopeDb    = 0.0f;
    float m_attackCoeff   = 0.0f;
    float m_releaseCoeff  = 0.0f;
};

} // namespace job::sound