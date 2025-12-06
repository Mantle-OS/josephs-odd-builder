#include "router_impl.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstring>

namespace job::ai::router {

namespace {
// Helper: Fast Top-K selection without full sort
void findTopK(const float* row, int n, int k, std::vector<std::pair<float, int>>& topk) {
    topk.clear();
    for (int i = 0; i < n; ++i) {
        float val = row[i];
        if (static_cast<int>(topk.size()) < k) {
            topk.push_back({val, i});
            if (static_cast<int>(topk.size()) == k) {
                std::sort(topk.begin(), topk.end(), [](auto& a, auto& b){ return a.first > b.first; });
            }
        } else {
            if (val > topk.back().first) {
                topk.back() = {val, i};
                std::sort(topk.begin(), topk.end(), [](auto& a, auto& b){ return a.first > b.first; });
            }
        }
    }
}

// Helper to look up adapter type for an expert ID
adapters::AdapterType getAdapterForExpert(const RouterConfig& cfg, int expertId) {
    // Safety check: if experts vector is empty or ID out of bounds, return default
    if (static_cast<size_t>(expertId) < cfg.experts.size())
        return cfg.experts[expertId].adapter;

    return adapters::AdapterType::Dense; // Default fallback
}

} // namespace

RouterPlan routeTopK(const RouterConfig &cfg, const RouterPlan &basePlan, const float *logits)
{
    RouterPlan result = basePlan;
    int batch = result.batchSize;
    int experts = result.numExperts;
    int k = cfg.topK;

    result.tokens.reserve(batch * k);
    std::vector<std::pair<float, int>> best;
    best.reserve(k + 1);

    for (int b = 0; b < batch; ++b) {
        const float* rowLogits = logits + b * experts;

        findTopK(rowLogits, experts, k, best);

        float maxVal = best[0].first;
        float sumExp = 0.0f;
        for (auto& pair : best) {
            pair.first = std::exp(pair.first - maxVal);
            sumExp += pair.first;
        }

        float invSum = 1.0f / sumExp;
        for (auto& pair : best) {
            RouterToken tok;
            tok.row = b;
            tok.expert = pair.second;
            tok.weight = pair.first * invSum;
            tok.adapter = getAdapterForExpert(cfg, tok.expert);

            result.tokens.push_back(tok);
        }
    }
    return result;
}

RouterPlan routeHash(const RouterConfig &cfg, const RouterPlan &basePlan, const cords::ViewR &input)
{
    RouterPlan result = basePlan;
    int batch = result.batchSize;
    int experts = result.numExperts;
    int k = cfg.topK;

    const float* data = input.data();
    int dim = input.extent()[1];

    for (int b = 0; b < batch; ++b) {
        uint32_t h = 2166136261u;
        const float* row = data + b * dim;
        int scanDim = std::min(dim, 8);

        for(int i=0; i<scanDim; ++i) {
            uint32_t bits;
            std::memcpy(&bits, &row[i], 4);
            h ^= bits;
            h *= 16777619u;
        }

        for (int i = 0; i < k; ++i) {
            RouterToken tok;
            tok.row = b;
            tok.expert = (h + i) % experts;
            tok.weight = 1.0f / k;
            tok.adapter = getAdapterForExpert(cfg, tok.expert);

            result.tokens.push_back(tok);
        }
    }
    return result;
}

RouterPlan routeSpatial(const RouterConfig &cfg, const RouterPlan &basePlan, const cords::ViewR &)
{
    RouterPlan result = basePlan;
    for (int b = 0; b < result.batchSize; ++b) {
        RouterToken tok;
        tok.row = b;
        tok.expert = 0;
        tok.weight = 1.0f;
        tok.adapter = getAdapterForExpert(cfg, 0);
        result.tokens.push_back(tok);
    }
    return result;
}

RouterPlan routeState(const RouterConfig &cfg, const RouterPlan &basePlan, const cords::ViewR &)
{
    RouterPlan result = basePlan;
    int experts = result.numExperts;

    for (int b = 0; b < result.batchSize; ++b) {
        RouterToken tok;
        tok.row = b;
        tok.expert = b % experts;
        tok.weight = 1.0f;
        tok.adapter = getAdapterForExpert(cfg, tok.expert);
        result.tokens.push_back(tok);
    }
    return result;
}

} // namespace job::ai::router
