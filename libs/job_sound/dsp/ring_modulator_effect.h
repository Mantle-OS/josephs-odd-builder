#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT RingModulatorEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<RingModulatorEffect>;
    using WPtr = std::weak_ptr<RingModulatorEffect>;
    using UPtr = std::unique_ptr<RingModulatorEffect>;

    static Ptr createShared() { return std::make_shared<RingModulatorEffect>(); }
    static UPtr createUnique() { return std::make_unique<RingModulatorEffect>(); }

    enum class CarrierWaveform {
        Sine,
        Triangle,
        Square
    };

    explicit RingModulatorEffect() :
        AudioEffect{"ring_modulator"}
    {
    }

    RingModulatorEffect(const RingModulatorEffect &) = default;
    RingModulatorEffect(RingModulatorEffect &&) noexcept = default;

    RingModulatorEffect &operator=(const RingModulatorEffect &) = default;
    RingModulatorEffect &operator=(RingModulatorEffect &&) noexcept = default;

    ~RingModulatorEffect() override = default;

    [[nodiscard]] float frequency() const noexcept { return m_frequency; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] CarrierWaveform waveform() const noexcept { return m_waveform; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Carrier frequency in Hz [0.5 Hz (slow tremolo-like), 5000.0 Hz (high sci-fi ring)]
    void setFrequency(float freqHz) noexcept
    {
        m_frequency = std::clamp(freqHz, 0.5f, m_sampleRate * 0.45f);
    }

    // Carrier waveform shape (Sine, Triangle, Square)
    void setWaveform(CarrierWaveform shape) noexcept
    {
        m_waveform = shape;
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
        m_phase = 0.0f;
    }

    [[nodiscard]] float process(float input) override final
    {
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;

        // Generate bipolar carrier sample in [-1.0f, 1.0f]
        float carrier = 0.0f;
        switch (m_waveform) {
        case CarrierWaveform::Sine:
            carrier = std::sin(m_phase);
            break;
        case CarrierWaveform::Triangle: {
            const float normPhase = m_phase / twoPi; // [0.0, 1.0)
            carrier = 2.0f * (1.0f - std::abs(2.0f * normPhase - 1.0f)) - 1.0f;
            break;
        }
        case CarrierWaveform::Square:
            carrier = (m_phase < std::numbers::pi_v<float>) ? 1.0f : -1.0f;
            break;
        }

        // Advance carrier oscillator phase
        m_phase += twoPi * m_frequency / m_sampleRate;
        if (m_phase >= twoPi)
            m_phase -= twoPi;

        // Four-quadrant multiplication
        const float modulated = input * carrier;

        // Dry / Wet Crossfade
        return (input * (1.0f - m_mix)) + (modulated * m_mix);
    }

private:
    float m_frequency           = 440.0f;  // Hz (A4 carrier default)
    float m_mix                 = 1.0f;    // 100% wet default
    CarrierWaveform m_waveform  = CarrierWaveform::Sine;
    float m_sampleRate          = 48000.0f;

    float m_phase               = 0.0f;
};

} // namespace job::sound