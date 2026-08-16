#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "config/sampler_config.h"
#include "jobmodel_export.h"

namespace job::ggml {
class JobGgmlTensor;
}

namespace job::model {

struct TokenCandidate {
    int32_t id;
    float   logit;
    float   probability;
};

class JOBMODEL_EXPORT Sampler {
public:
    using Ptr  = std::shared_ptr<Sampler>;
    using UPtr = std::unique_ptr<Sampler>;

    explicit Sampler(const SamplerConfig& config = {});
    ~Sampler() = default;

    [[nodiscard]] static Ptr createShared(const SamplerConfig& config = {})
    {
        return std::make_shared<Sampler>(config);
    }

    [[nodiscard]] static UPtr createUniq(const SamplerConfig& config = {})
    {
        return std::make_unique<Sampler>(config);
    }

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler(Sampler&&) noexcept = default;
    Sampler& operator=(Sampler&&) noexcept = default;

    // Sample a token ID from raw logits tensor and context history
    [[nodiscard]] int32_t sample(
        ggml::JobGgmlTensor& logitsTensor,
        std::span<const int32_t> contextTokens = {});

    void setConfig(const SamplerConfig& config) noexcept { m_config = config; }
    [[nodiscard]] const SamplerConfig& config() const noexcept { return m_config; }

    void seed(uint64_t seedValue) noexcept
    {
        m_config.m_seed = seedValue;
        m_rngState = seedValue;
    }

private:
    float nextRandomFloat() noexcept;

    void applyPenalties(std::vector<TokenCandidate>& candidates, std::span<const int32_t> contextTokens) const;
    void applyTemperature(std::vector<TokenCandidate>& candidates) const;
    void computeSoftmax(std::vector<TokenCandidate>& candidates) const;
    void filterTopK(std::vector<TokenCandidate>& candidates) const;
    void filterTopP(std::vector<TokenCandidate>& candidates) const;
    void filterMinP(std::vector<TokenCandidate>& candidates) const;
    int32_t sampleIndex(const std::vector<TokenCandidate>& candidates);

    SamplerConfig m_config;
    uint64_t      m_rngState{1337};
};

} // namespace job::model