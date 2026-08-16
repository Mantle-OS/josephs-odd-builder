#include "sampler.h"

#include <algorithm>
#include <cmath>

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_data.h>
#include <job_logger.h>

namespace job::model {

Sampler::Sampler(const SamplerConfig& config)
    : m_config(config)
    , m_rngState(config.m_seed)
{
}

float Sampler::nextRandomFloat() noexcept
{
    // PCG / Xorshift64 generator for deterministic, high-performance sampling
    m_rngState ^= m_rngState >> 12;
    m_rngState ^= m_rngState << 25;
    m_rngState ^= m_rngState >> 27;
    constexpr uint64_t kMultiplier = 2685821657736338717ULL;
    uint64_t val = m_rngState * kMultiplier;
    return static_cast<float>(val >> 11) / static_cast<float>(1ULL << 53);
}

int32_t Sampler::sample(
    ggml::JobGgmlTensor& logitsTensor,
    std::span<const int32_t> contextTokens)
{
    if (!logitsTensor.isValid()) {
        JOB_LOG_ERROR("[Sampler] Cannot sample from an invalid logits tensor");
        return -1;
    }

    ggml::JobGgmlTensorData tensorData(logitsTensor.tensor());
    if (!tensorData.isHostAccessible()) {
        JOB_LOG_ERROR("[Sampler] Logits tensor storage is not host accessible");
        return -1;
    }

    const int64_t vocabSize = logitsTensor.extent(0);
    if (vocabSize <= 0) {
        JOB_LOG_ERROR("[Sampler] Logits tensor vocabulary size is zero or negative");
        return -1;
    }

    // Extract logits for the final sequence token in the batch
    std::vector<TokenCandidate> candidates;
    candidates.reserve(static_cast<size_t>(vocabSize));

    for (int64_t i = 0; i < vocabSize; ++i) {
        float val = tensorData.valueF32(i);
        candidates.push_back(TokenCandidate{
            .id = static_cast<int32_t>(i),
            .logit = val,
            .probability = 0.0f
        });
    }

    // Greedy argmax selection shortcut
    if (m_config.m_greedy || m_config.m_temperature <= 0.0f) {
        auto best = std::max_element(candidates.begin(), candidates.end(),
            [](const TokenCandidate& a, const TokenCandidate& b) {
                return a.logit < b.logit;
            });
        return best != candidates.end() ? best->id : 0;
    }

    // Repetition, Frequency, and Presence Penalties
    applyPenalties(candidates, contextTokens);

    // Temperature Scaling
    applyTemperature(candidates);

    // Softmax Normalization
    computeSoftmax(candidates);

    // Truncation Filters (Top-K, Top-P, Min-P)
    filterTopK(candidates);
    filterTopP(candidates);
    filterMinP(candidates);

    // If all candidates got filtered out, fall back to argmax
    if (candidates.empty()) {
        auto best = std::max_element(candidates.begin(), candidates.end(),
            [](const TokenCandidate& a, const TokenCandidate& b) {
                return a.logit < b.logit;
            });
        return best != candidates.end() ? best->id : 0;
    }

    // Stochastic sampling from filtered distribution
    return sampleIndex(candidates);
}

void Sampler::applyPenalties(
    std::vector<TokenCandidate>& candidates,
    std::span<const int32_t> contextTokens) const
{
    if (contextTokens.empty()) return;

    for (int32_t token : contextTokens) {
        if (token >= 0 && static_cast<size_t>(token) < candidates.size()) {
            auto& cand = candidates[static_cast<size_t>(token)];
            // Repetition penalty (multiplicative on positive logits, divisive on negative)
            if (m_config.m_repeatPenalty != 1.0f) {
                if (cand.logit < 0.0f) {
                    cand.logit *= m_config.m_repeatPenalty;
                } else {
                    cand.logit /= m_config.m_repeatPenalty;
                }
            }
        }
    }

    // Frequency and presence penalties
    if (m_config.m_frequencyPenalty != 0.0f || m_config.m_presencePenalty != 0.0f) {
        std::vector<int> counts(candidates.size(), 0);
        for (int32_t token : contextTokens) {
            if (token >= 0 && static_cast<size_t>(token) < counts.size()) {
                counts[static_cast<size_t>(token)]++;
            }
        }

        for (size_t i = 0; i < candidates.size(); ++i) {
            int count = counts[i];
            if (count > 0) {
                candidates[i].logit -= static_cast<float>(count) * m_config.m_frequencyPenalty;
                candidates[i].logit -= m_config.m_presencePenalty;
            }
        }
    }
}

void Sampler::applyTemperature(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.m_temperature == 1.0f) return;

    float invTemp = 1.0f / m_config.m_temperature;
    for (auto& cand : candidates) {
        cand.logit *= invTemp;
    }
}

void Sampler::computeSoftmax(std::vector<TokenCandidate>& candidates) const
{
    if (candidates.empty()) return;

    float maxLogit = std::max_element(candidates.begin(), candidates.end(),
        [](const TokenCandidate& a, const TokenCandidate& b) {
            return a.logit < b.logit;
        })->logit;

    float sumExp = 0.0f;
    for (auto& cand : candidates) {
        cand.probability = std::exp(cand.logit - maxLogit);
        sumExp += cand.probability;
    }

    if (sumExp > 0.0f) {
        float invSum = 1.0f / sumExp;
        for (auto& cand : candidates) {
            cand.probability *= invSum;
        }
    }
}

void Sampler::filterTopK(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.m_topK <= 0 || static_cast<size_t>(m_config.m_topK) >= candidates.size()) {
        return;
    }

    // Sort descending by probability/logit
    std::sort(candidates.begin(), candidates.end(),
        [](const TokenCandidate& a, const TokenCandidate& b) {
            return a.logit > b.logit;
        });

    candidates.resize(static_cast<size_t>(m_config.m_topK));
}

void Sampler::filterTopP(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.m_topP >= 1.0f || candidates.empty()) return;

    // Ensure sorted descending
    std::sort(candidates.begin(), candidates.end(),
        [](const TokenCandidate& a, const TokenCandidate& b) {
            return a.logit > b.logit;
        });

    float cumulativeProb = 0.0f;
    size_t cutoffIndex = candidates.size();

    for (size_t i = 0; i < candidates.size(); ++i) {
        cumulativeProb += candidates[i].probability;
        if (cumulativeProb > m_config.m_topP) {
            cutoffIndex = i + 1; // Keep at least one token exceeding threshold
            break;
        }
    }

    candidates.resize(cutoffIndex);
}

void Sampler::filterMinP(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.m_minP <= 0.0f || candidates.empty()) return;

    // Find highest probability
    auto maxIt = std::max_element(candidates.begin(), candidates.end(),
        [](const TokenCandidate& a, const TokenCandidate& b) {
            return a.probability < b.probability;
        });

    if (maxIt == candidates.end()) return;
    float maxProb = maxIt->probability;
    float threshold = maxProb * m_config.m_minP;

    candidates.erase(
        std::remove_if(candidates.begin(), candidates.end(),
            [threshold](const TokenCandidate& cand) {
                return cand.probability < threshold;
            }),
        candidates.end()
    );
}

int32_t Sampler::sampleIndex(const std::vector<TokenCandidate>& candidates)
{
    // Re-normalize probabilities after filtering
    float sumProb = 0.0f;
    for (const auto& cand : candidates) {
        sumProb += cand.probability;
    }

    if (sumProb <= 0.0f) {
        return candidates[0].id;
    }

    float roll = nextRandomFloat() * sumProb;
    float cumulative = 0.0f;

    for (const auto& cand : candidates) {
        cumulative += cand.probability;
        if (roll <= cumulative) {
            return cand.id;
        }
    }

    return candidates.back().id;
}

} // namespace job::model