#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT BitcrusherEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<BitcrusherEffect>;
    using WPtr = std::weak_ptr<BitcrusherEffect>;
    using UPtr = std::unique_ptr<BitcrusherEffect>;

    static Ptr createShared() { return std::make_shared<BitcrusherEffect>(); }
    static UPtr createUnique() { return std::make_unique<BitcrusherEffect>(); }

    explicit BitcrusherEffect() :
        AudioEffect{"bitcrusher"}
    {
        updateQuantizationSteps();
    }

    BitcrusherEffect(const BitcrusherEffect &) = default;
    BitcrusherEffect(BitcrusherEffect &&) noexcept = default;

    BitcrusherEffect &operator=(const BitcrusherEffect &) = default;
    BitcrusherEffect &operator=(BitcrusherEffect &&) noexcept = default;

    ~BitcrusherEffect() override = default;

    [[nodiscard]] float bitDepth() const noexcept { return m_bitDepth; }
    [[nodiscard]] float downsampleFactor() const noexcept { return m_downsampleFactor; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Bit depth resolution [1.0 bit, 24.0 bits]
    void setBitDepth(float bits) noexcept
    {
        m_bitDepth = std::clamp(bits, 1.0f, 24.0f);
        updateQuantizationSteps();
    }

    // Downsample factor [1.0 (no decimation), 64.0 (extreme downsampling)]
    void setDownsampleFactor(float factor) noexcept
    {
        m_downsampleFactor = std::max(1.0f, factor);
    }

    // Target downsampled frequency in Hz (converts to downsample factor relative to sampleRate)
    void setTargetSampleRate(float targetRateHz) noexcept
    {
        if (targetRateHz > 0.0f)
            setDownsampleFactor(m_sampleRate / targetRateHz);
    }

    // Dry / Wet Mix [0.0f dry, 1.0f wet]
    void setMix(float mix) noexcept
    {
        m_mix = std::clamp(mix, 0.0f, 1.0f);
    }

    void setSampleRate(float sampleRate) noexcept
    {
        if (sampleRate > 0.0f)
            m_sampleRate = sampleRate;
    }

    void reset() noexcept
    {
        m_phaseAcc = 0.0f;
        m_lastCrushedSample = 0.0f;
    }

    [[nodiscard]] float process(float input) override final
    {
        // Zero-Order Hold / Downsampling accumulation
        m_phaseAcc += 1.0f;
        if (m_phaseAcc >= m_downsampleFactor) {
            m_phaseAcc -= m_downsampleFactor;

            // Amplitude quantization (Bit reduction)
            const float clampedInput = std::clamp(input, -1.0f, 1.0f);
            m_lastCrushedSample = std::round(clampedInput * m_quantSteps) / m_quantSteps;
        }

        // Dry / Wet Crossfade
        return (input * (1.0f - m_mix)) + (m_lastCrushedSample * m_mix);
    }

private:
    void updateQuantizationSteps() noexcept
    {
        // Total steps = 2^(bits - 1)
        m_quantSteps = std::pow(2.0f, m_bitDepth - 1.0f);
    }

private:
    float m_bitDepth          = 8.0f;    // 8-bit default
    float m_downsampleFactor  = 4.0f;    // 4x downsample (e.g. 48kHz -> 12kHz)
    float m_mix               = 1.0f;    // [0.0 dry, 1.0 wet]
    float m_sampleRate        = 48000.0f;

    float m_quantSteps        = 128.0f;
    float m_phaseAcc          = 0.0f;
    float m_lastCrushedSample = 0.0f;
};

} // namespace job::sound