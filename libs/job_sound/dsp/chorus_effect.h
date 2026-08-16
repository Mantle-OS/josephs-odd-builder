#pragma once

#include <cmath>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT ChorusEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<ChorusEffect>;
    using WPtr = std::weak_ptr<ChorusEffect>;
    using UPtr = std::unique_ptr<ChorusEffect>;

    static Ptr createShared() { return std::make_shared<ChorusEffect>(); }
    static UPtr createUnique() { return std::make_unique<ChorusEffect>(); }

    explicit ChorusEffect() :
        AudioEffect{"chorus"},
        m_phase(0.0f),
        m_maxDelay(2048)
    {
        m_buffer.resize(m_maxDelay);
    }

    ChorusEffect(const ChorusEffect &) = default;
    ChorusEffect(ChorusEffect &&) noexcept = default;

    ChorusEffect &operator=(const ChorusEffect &) = default;
    ChorusEffect &operator=(ChorusEffect &&) noexcept = default;

    ~ChorusEffect() override = default;

    [[nodiscard]] float depth() const noexcept { return m_depth; }
    [[nodiscard]] float rate() const noexcept { return m_rate; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    void setDepth(float depth) noexcept { m_depth = depth; }
    void setRate(float rate) noexcept { m_rate = rate; }
    void setMix(float mix) noexcept { m_mix = mix; }
    void setSampleRate(float sampleRate) noexcept { m_sampleRate = sampleRate; }

    // [[FIXME]] m_depth * m_sampleRate must remain below m_maxDelay,
    // otherwise the delay index math can wrap to the wrong delay position.
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

        m_buffer[m_writeIndex % m_maxDelay] = input;
        m_writeIndex = (m_writeIndex + 1) % m_maxDelay;

        return output;
    }

private:
    float m_depth      = 0.005f;   // seconds
    float m_rate       = 0.8f;     // Hz
    float m_mix        = 0.5f;
    float m_sampleRate = 48000.0f;

    std::vector<float> m_buffer;

    int   m_writeIndex = 0;
    float m_phase      = 0.0f;
    int   m_maxDelay   = 2048;
};

} // namespace job::sound