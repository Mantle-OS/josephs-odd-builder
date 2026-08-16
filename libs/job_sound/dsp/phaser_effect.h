#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT PhaserEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<PhaserEffect>;
    using WPtr = std::weak_ptr<PhaserEffect>;
    using UPtr = std::unique_ptr<PhaserEffect>;

    static Ptr createShared() { return std::make_shared<PhaserEffect>(); }
    static UPtr createUnique() { return std::make_unique<PhaserEffect>(); }

    explicit PhaserEffect() :
        AudioEffect{"phaser"}
    {
    }

    PhaserEffect(const PhaserEffect &) = default;
    PhaserEffect(PhaserEffect &&) noexcept = default;

    PhaserEffect &operator=(const PhaserEffect &) = default;
    PhaserEffect &operator=(PhaserEffect &&) noexcept = default;

    ~PhaserEffect() override = default;

    [[nodiscard]] float rate() const noexcept { return m_rate; }
    [[nodiscard]] float depth() const noexcept { return m_depth; }
    [[nodiscard]] float feedback() const noexcept { return m_feedback; }
    [[nodiscard]] float baseFrequency() const noexcept { return m_baseFreq; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Modulation rate in Hz [0.05 Hz, 10.0 Hz]
    void setRate(float rateHz) noexcept
    {
        m_rate = std::clamp(rateHz, 0.05f, 10.0f);
    }

    // Modulation depth / sweep range [0.0f, 1.0f]
    void setDepth(float depth) noexcept
    {
        m_depth = std::clamp(depth, 0.0f, 1.0f);
    }

    // Feedback resonance [-0.95f, 0.95f]
    void setFeedback(float feedback) noexcept
    {
        m_feedback = std::clamp(feedback, -0.95f, 0.95f);
    }

    // Minimum center frequency in Hz [50.0 Hz, 2000.0 Hz]
    void setBaseFrequency(float baseFreqHz) noexcept
    {
        m_baseFreq = std::clamp(baseFreqHz, 50.0f, 2000.0f);
    }

    // Dry / Wet Mix [0.0f dry, 1.0f wet, 0.5f = maximum notch cancellation]
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
        m_lastFeedback = 0.0f;
        for (auto &stage : m_stages) {
            stage.x1 = 0.0f;
            stage.y1 = 0.0f;
        }
    }

    [[nodiscard]] float process(float input) override final
    {
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;

        // Calculate modulated cutoff frequency
        // LFO output [0.0f, 1.0f]
        const float lfo = 0.5f * (1.0f + std::sin(m_phase));

        // Sweep frequency exponentially across octaves
        const float sweepOctaves = 4.0f * m_depth;
        const float currentFreq = m_baseFreq * std::pow(2.0f, lfo * sweepOctaves);
        const float clampedFreq = std::clamp(currentFreq, 20.0f, m_sampleRate * 0.45f);

        // Compute 1st-order All-Pass Filter coefficient 'a'
        const float w = std::tan(std::numbers::pi_v<float> * clampedFreq / m_sampleRate);
        const float a = (w - 1.0f) / (w + 1.0f);

        // Advance LFO phase
        m_phase += twoPi * m_rate / m_sampleRate;
        if (m_phase >= twoPi)
            m_phase -= twoPi;

        // Cascade through 4 All-Pass Filter stages
        float stageInput = input + (m_lastFeedback * m_feedback);

        for (auto &stage : m_stages) {
            // Difference equation: y[n] = a * (x[n] - y[n-1]) + x[n-1]
            const float y = a * (stageInput - stage.y1) + stage.x1;
            stage.x1 = stageInput;
            stage.y1 = y;
            stageInput = y;
        }

        const float allpassOut = stageInput;
        m_lastFeedback = allpassOut;

        // Mix dry and wet signals (0.5 = complete phase cancellation at notches)
        return (input * (1.0f - m_mix)) + (allpassOut * m_mix);
    }

private:
    struct AllpassStage {
        float x1 = 0.0f;
        float y1 = 0.0f;
    };

    float m_rate         = 0.5f;     // Hz (slow sweep default)
    float m_depth        = 0.75f;    // [0.0, 1.0]
    float m_feedback     = 0.5f;     // [0.0, 0.95]
    float m_baseFreq     = 250.0f;   // Hz
    float m_mix          = 0.5f;     // 50/50 mix produces deep notches
    float m_sampleRate   = 48000.0f;

    float m_phase        = 0.0f;
    float m_lastFeedback = 0.0f;
    std::array<AllpassStage, 4> m_stages;
};

} // namespace job::sound