#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT TremoloEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<TremoloEffect>;
    using WPtr = std::weak_ptr<TremoloEffect>;
    using UPtr = std::unique_ptr<TremoloEffect>;

    static Ptr createShared() { return std::make_shared<TremoloEffect>(); }
    static UPtr createUnique() { return std::make_unique<TremoloEffect>(); }

    enum class Waveform {
        Sine,
        Triangle,
        Square
    };

    explicit TremoloEffect() :
        AudioEffect{"tremolo"}
    {
    }

    TremoloEffect(const TremoloEffect &) = default;
    TremoloEffect(TremoloEffect &&) noexcept = default;

    TremoloEffect &operator=(const TremoloEffect &) = default;
    TremoloEffect &operator=(TremoloEffect &&) noexcept = default;

    ~TremoloEffect() override = default;

    [[nodiscard]] float rate() const noexcept { return m_rate; }
    [[nodiscard]] float depth() const noexcept { return m_depth; }
    [[nodiscard]] Waveform waveform() const noexcept { return m_waveform; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Rate in Hz [0.1 Hz, 20.0 Hz]
    void setRate(float rateHz) noexcept
    {
        m_rate = std::clamp(rateHz, 0.1f, 20.0f);
    }

    // Depth [0.0f (no modulation), 1.0f (full amplitude drop)]
    void setDepth(float depth) noexcept
    {
        m_depth = std::clamp(depth, 0.0f, 1.0f);
    }

    void setWaveform(Waveform shape) noexcept
    {
        m_waveform = shape;
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

        // Calculate raw LFO output in range [0.0f, 1.0f]
        float lfo = 0.0f;
        switch (m_waveform) {
        case Waveform::Sine:
            lfo = 0.5f * (1.0f + std::sin(m_phase));
            break;
        case Waveform::Triangle: {
            const float normPhase = m_phase / twoPi; // [0.0, 1.0)
            lfo = 1.0f - std::abs(2.0f * normPhase - 1.0f);
            break;
        }
        case Waveform::Square:
            lfo = (m_phase < std::numbers::pi_v<float>) ? 1.0f : 0.0f;
            break;
        }

        // Advance LFO phase
        m_phase += twoPi * m_rate / m_sampleRate;
        if (m_phase >= twoPi)
            m_phase -= twoPi;

        // Amplitude modulation: gain = 1.0 - (depth * (1.0 - lfo))
        const float gain = 1.0f - m_depth * (1.0f - lfo);

        return input * gain;
    }

private:
    float m_rate       = 4.0f;     // Hz
    float m_depth      = 0.75f;    // [0.0, 1.0]
    Waveform m_waveform = Waveform::Sine;
    float m_sampleRate = 48000.0f;

    float m_phase      = 0.0f;
};

} // namespace job::sound