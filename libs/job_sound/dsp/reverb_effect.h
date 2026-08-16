#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "audio_effects.h"
#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT ReverbEffect final : public AudioEffect
{
public:
    using Ptr  = std::shared_ptr<ReverbEffect>;
    using WPtr = std::weak_ptr<ReverbEffect>;
    using UPtr = std::unique_ptr<ReverbEffect>;

    static Ptr createShared() { return std::make_shared<ReverbEffect>(); }
    static UPtr createUnique() { return std::make_unique<ReverbEffect>(); }

    explicit ReverbEffect() :
        AudioEffect{"reverb"}
    {
        initDelayLines();
    }

    ReverbEffect(const ReverbEffect &) = default;
    ReverbEffect(ReverbEffect &&) noexcept = default;

    ReverbEffect &operator=(const ReverbEffect &) = default;
    ReverbEffect &operator=(ReverbEffect &&) noexcept = default;

    ~ReverbEffect() override = default;

    [[nodiscard]] float roomSize() const noexcept { return m_roomSize; }
    [[nodiscard]] float damping() const noexcept { return m_damping; }
    [[nodiscard]] float mix() const noexcept { return m_mix; }
    [[nodiscard]] float sampleRate() const noexcept { return m_sampleRate; }

    // Room size [0.0f (small studio), 0.98f (cavernous hall)]
    void setRoomSize(float size) noexcept
    {
        m_roomSize = std::clamp(size, 0.0f, 0.98f);
    }

    // High frequency damping factor [0.0f (bright reflections), 1.0f (dark/warm absorption)]
    void setDamping(float damping) noexcept
    {
        m_damping = std::clamp(damping, 0.0f, 1.0f);
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
            initDelayLines();
        }
    }

    void reset() noexcept
    {
        for (auto &comb : m_combs) {
            std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
            comb.filterStore = 0.0f;
            comb.index = 0;
        }
        for (auto &allpass : m_allpasses) {
            std::fill(allpass.buffer.begin(), allpass.buffer.end(), 0.0f);
            allpass.index = 0;
        }
    }

    [[nodiscard]] float process(float input) override final
    {
        const float scaledInput = input * 0.015f; // Headroom attenuation
        float combOut = 0.0f;

        // Process 8 Parallel Comb Filters
        for (auto &comb : m_combs) {
            const float output = comb.buffer[comb.index];
            comb.filterStore = (output * (1.0f - m_damping)) + (comb.filterStore * m_damping);
            comb.buffer[comb.index] = scaledInput + (comb.filterStore * m_roomSize);

            if (++comb.index >= comb.buffer.size())
                comb.index = 0;

            combOut += output;
        }

        // Process 4 Series Allpass Diffusers
        float allpassOut = combOut;
        for (auto &allpass : m_allpasses) {
            const float bufOut = allpass.buffer[allpass.index];
            const float feedbackOut = allpassOut + (bufOut * 0.5f);
            allpass.buffer[allpass.index] = feedbackOut;

            allpassOut = -allpassOut + bufOut;

            if (++allpass.index >= allpass.buffer.size())
                allpass.index = 0;
        }

        return (input * (1.0f - m_mix)) + (allpassOut * m_mix * 2.0f);
    }

private:
    struct CombFilter {
        std::vector<float> buffer;
        float filterStore = 0.0f;
        std::size_t index = 0;
    };

    struct AllpassFilter {
        std::vector<float> buffer;
        std::size_t index = 0;
    };

    void initDelayLines()
    {
        // Standard Schroeder/Freeverb prime-based delay lengths scaled to sample rate
        constexpr std::array<int, 8> kCombTunings = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        constexpr std::array<int, 4> kAllpassTunings = {556, 441, 341, 225};

        const float scale = m_sampleRate / 44100.0f;

        for (std::size_t i = 0; i < m_combs.size(); ++i) {
            const auto size = static_cast<std::size_t>(std::round(static_cast<float>(kCombTunings[i]) * scale));
            m_combs[i].buffer.assign(std::max<std::size_t>(size, 1), 0.0f);
            m_combs[i].filterStore = 0.0f;
            m_combs[i].index = 0;
        }

        for (std::size_t i = 0; i < m_allpasses.size(); ++i) {
            const auto size = static_cast<std::size_t>(std::round(static_cast<float>(kAllpassTunings[i]) * scale));
            m_allpasses[i].buffer.assign(std::max<std::size_t>(size, 1), 0.0f);
            m_allpasses[i].index = 0;
        }
    }

private:
    float m_roomSize   = 0.5f;   // [0.0, 0.98]
    float m_damping    = 0.2f;   // [0.0, 1.0]
    float m_mix        = 0.35f;  // [0.0, 1.0]
    float m_sampleRate = 48000.0f;

    std::array<CombFilter, 8> m_combs;
    std::array<AllpassFilter, 4> m_allpasses;
};

} // namespace job::sound