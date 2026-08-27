#pragma once

#include <cstdint>
#include <memory>

#include <job_base_obj.h>

#include "jobmodel_export.h"

namespace job::model {

// The control panel for token generation.
// Because without these knobs, your model either loops the same phrase
// into infinity or hallucinates poetry about toaster manuals.
class JOBMODEL_EXPORT SamplerConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<SamplerConfig>;
    using WPtr = std::weak_ptr<SamplerConfig>;
    using UPtr = std::unique_ptr<SamplerConfig>;

    SamplerConfig();
    ~SamplerConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<SamplerConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<SamplerConfig>(); }

    SamplerConfig(const SamplerConfig &) = default;
    SamplerConfig &operator=(const SamplerConfig &) = default;
    SamplerConfig(SamplerConfig &&) noexcept = default;
    SamplerConfig &operator=(SamplerConfig &&) noexcept = default;

    [[nodiscard]] float temperature() const noexcept { return m_temperature; }
    void setTemperature(float value) noexcept { m_temperature = value; }

    [[nodiscard]] int32_t topK() const noexcept { return m_topK; }
    void setTopK(int32_t value) noexcept { m_topK = value; }

    [[nodiscard]] float topP() const noexcept { return m_topP; }
    void setTopP(float value) noexcept { m_topP = value; }

    [[nodiscard]] float minP() const noexcept { return m_minP; }
    void setMinP(float value) noexcept { m_minP = value; }

    [[nodiscard]] float repeatPenalty() const noexcept { return m_repeatPenalty; }
    void setRepeatPenalty(float value) noexcept { m_repeatPenalty = value; }

    [[nodiscard]] float frequencyPenalty() const noexcept { return m_frequencyPenalty; }
    void setFrequencyPenalty(float value) noexcept { m_frequencyPenalty = value; }

    [[nodiscard]] float presencePenalty() const noexcept { return m_presencePenalty; }
    void setPresencePenalty(float value) noexcept { m_presencePenalty = value; }

    [[nodiscard]] uint64_t seed() const noexcept { return m_seed; }
    void setSeed(uint64_t value) noexcept { m_seed = value; }

    [[nodiscard]] bool greedy() const noexcept { return m_greedy; }
    void setGreedy(bool value) noexcept { m_greedy = value; }

    [[nodiscard]] bool isValid() const noexcept;

private:
    float    m_temperature{0.8f};
    int32_t  m_topK{40};
    float    m_topP{0.95f};
    float    m_minP{0.05f};
    float    m_repeatPenalty{1.1f};
    float    m_frequencyPenalty{0.0f};
    float    m_presencePenalty{0.0f};
    uint64_t m_seed{1337};
    bool     m_greedy{false};
};

} // namespace job::model
