#include "sampler.h"

#include <algorithm>
#include <cmath>

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_data.h>
#include <job_logger.h>

namespace job::model {

Sampler::Sampler(const SamplerConfig& config) :
    m_config(config),
    m_rngState(config.seed())
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
int32_t Sampler::sample(std::span<const float> logits,
                        std::span<const int32_t> contextTokens)
{
    if (logits.empty()) {
        JOB_LOG_ERROR("[Sampler] Cannot sample from empty logits");
        return -1;
    }

    std::vector<TokenCandidate> candidates;
    candidates.reserve(logits.size());

    for (size_t i = 0; i < logits.size(); ++i) {
        candidates.push_back(TokenCandidate{
            .id = static_cast<int32_t>(i),
            .logit = logits[i],
            .probability = 0.0f
        });
    }

    // Apply penalties before greedy selection.
    applyPenalties(candidates, contextTokens);

    const auto best = std::max_element(candidates.begin(), candidates.end(),
                                       [](const TokenCandidate& a, const TokenCandidate& b) {
                                           return a.logit < b.logit;
                                       });

    const int32_t fallbackToken = best->id;

    if (m_config.greedy() || m_config.temperature() <= 0.0f)
        return fallbackToken;

    // Top-K can operate directly on logits.
    filterTopK(candidates);

    // Scale surviving logits by temperature.
    applyTemperature(candidates);

    // Normalize the remaining candidates.
    computeSoftmax(candidates);

    // Probability-based filtering.
    filterTopP(candidates);
    filterMinP(candidates);

    if (candidates.empty())
        return fallbackToken;

    return sampleIndex(candidates);
}

int32_t Sampler::sample(ggml::JobGgmlTensor& logitsTensor,
                        std::span<const int32_t> contextTokens)
{
    if (!logitsTensor.isValid()) {
        JOB_LOG_ERROR("[Sampler] Cannot sample from an invalid logits tensor");
        return -1;
    }

    if (logitsTensor.type() != ggml::JobGgmlType::F32) {
        JOB_LOG_ERROR("[Sampler] Requires F32 logits");
        return -1;
    }

    ggml::JobGgmlTensorData tensorData(logitsTensor.tensor());

    if (!tensorData.isHostAccessible()) {
        JOB_LOG_ERROR("[Sampler] Logits tensor storage is not host accessible");
        return -1;
    }

    const int64_t vocabSize = logitsTensor.extent(0);

    if (vocabSize <= 0) {
        JOB_LOG_ERROR("[Sampler] Invalid vocabulary size");
        return -1;
    }

    std::vector<float> logits(static_cast<size_t>(vocabSize));

    for (int64_t i = 0; i < vocabSize; ++i)
        logits[static_cast<size_t>(i)] = tensorData.valueF32(i);

    return sample(std::span<const float>{logits}, contextTokens);
}


void Sampler::applyPenalties(std::vector<TokenCandidate>& candidates, std::span<const int32_t> contextTokens) const
{
    if (contextTokens.empty() || candidates.empty())
        return;

    const float repeatPenalty = m_config.repeatPenalty();
    const float frequencyPenalty = m_config.frequencyPenalty();
    const float presencePenalty = m_config.presencePenalty();

    if (repeatPenalty == 1.0f && frequencyPenalty == 0.0f && presencePenalty == 0.0f)
        return;

    std::vector<uint32_t> counts(candidates.size(), 0);

    for (const int32_t token : contextTokens) {
        if (token >= 0 && static_cast<size_t>(token) < counts.size())
            ++counts[static_cast<size_t>(token)];
    }

    for (size_t i = 0; i < candidates.size(); ++i) {
        const uint32_t count = counts[i];
        if (count == 0)
            continue;

        auto &candidate = candidates[i];

        if (repeatPenalty != 1.0f) {
            if (candidate.logit < 0.0f)
                candidate.logit *= repeatPenalty;
            else
                candidate.logit /= repeatPenalty;
        }

        candidate.logit -= static_cast<float>(count) * frequencyPenalty;
        candidate.logit -= presencePenalty;
    }
}

void Sampler::applyTemperature(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.temperature() == 1.0f)
        return;

    float invTemp = 1.0f / m_config.temperature();
    for (auto& cand : candidates)
        cand.logit *= invTemp;
}

void Sampler::computeSoftmax(std::vector<TokenCandidate>& candidates) const
{
    if (candidates.empty())
        return;

    float maxLogit = std::max_element(candidates.begin(), candidates.end(), [](const TokenCandidate& a, const TokenCandidate& b) {
                         return a.logit < b.logit;
                     })->logit;

    float sumExp = 0.0f;
    for (auto& cand : candidates) {
        cand.probability = std::exp(cand.logit - maxLogit); // <<< Damn it exp is known to be slow
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
    if (m_config.topK() <= 0 || static_cast<size_t>(m_config.topK()) >= candidates.size()) {
        return;
    }

    // Sort descending by probability/logit
    std::sort(candidates.begin(), candidates.end(), [](const TokenCandidate& a, const TokenCandidate& b) {
        return a.logit > b.logit;
    });

    candidates.resize(static_cast<size_t>(m_config.topK()));
}

void Sampler::filterTopP(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.topP() >= 1.0f || candidates.empty()) return;

    // Ensure sorted descending
    std::sort(candidates.begin(), candidates.end(), [](const TokenCandidate& a, const TokenCandidate& b) {
        return a.logit > b.logit;
    });

    float cumulativeProb = 0.0f;
    size_t cutoffIndex = candidates.size();

    for (size_t i = 0; i < candidates.size(); ++i) {
        cumulativeProb += candidates[i].probability;
        if (cumulativeProb > m_config.topP()) {
            cutoffIndex = i + 1; // Keep at least one token exceeding threshold
            break;
        }
    }

    candidates.resize(cutoffIndex);
}

void Sampler::filterMinP(std::vector<TokenCandidate>& candidates) const
{
    if (m_config.minP() <= 0.0f || candidates.empty())
        return;

    // Find highest probability
    auto maxIt = std::max_element(candidates.begin(), candidates.end(), [](const TokenCandidate& a, const TokenCandidate& b) {
        return a.probability < b.probability;
    });

    if (maxIt == candidates.end()) return;
    float maxProb = maxIt->probability;
    float threshold = maxProb * m_config.minP();

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