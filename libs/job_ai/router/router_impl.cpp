#include "router_impl.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstring>

namespace job::ai::router {

namespace {
// Helper: Fast Top-K selection without full sort
// Used when K is small (1, 2, 4)
void findTopK(const float* row, int n, int k, std::vector<std::pair<float, int>>& topk) {
    topk.clear();
    // Scan all experts
    for (int i = 0; i < n; ++i) {
        float val = row[i];

        if (static_cast<int>(topk.size()) < k) {
            topk.push_back({val, i});
            // Keep sorted descending if full
            if (static_cast<int>(topk.size()) == k) {
                std::sort(topk.begin(), topk.end(), [](auto& a, auto& b){ return a.first > b.first; });
            }
        } else {
            // If valid candidate (better than worst in topk)
            if (val > topk.back().first) {
                topk.back() = {val, i};
                // Re-sort to keep smallest at back
                std::sort(topk.begin(), topk.end(), [](auto& a, auto& b){ return a.first > b.first; });
            }
        }
    }
}
}

RouterPlan routeTopK(const RouterConfig &cfg, const RouterPlan &basePlan, const float *logits)
{
    RouterPlan result = basePlan;
    int batch = result.batchSize;
    int experts = result.numExperts;
    int k = cfg.topK;

    // Reserve optimization
    result.tokens.reserve(batch * k);

    // Per-thread scratchpad if we parallelized, but serial for now
    std::vector<std::pair<float, int>> best;
    best.reserve(k + 1);

    for (int b = 0; b < batch; ++b) {
        const float* rowLogits = logits + b * experts;

        findTopK(rowLogits, experts, k, best);

        // Softmax over Top-K
        // 1. Max for stability
        float maxVal = best[0].first; // Sorted descending, first is max

        // 2. Sum Exp
        float sumExp = 0.0f;
        for (auto& pair : best) {
            // Reuse pair.first to store exp value to avoid recomputing
            pair.first = std::exp(pair.first - maxVal);
            sumExp += pair.first;
        }

        // 3. Normalize and Append
        float invSum = 1.0f / sumExp;
        for (auto& pair : best) {
            RouterToken tok;
            tok.row = b;
            tok.expert = pair.second;
            tok.weight = pair.first * invSum;
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
        // Deterministic Hashing
        // FNV-1a style hash on the first few dimensions of the input vector
        uint32_t h = 2166136261u;
        const float* row = data + b * dim;
        int scanDim = std::min(dim, 8); // Only hash first 8 floats for speed

        for(int i=0; i<scanDim; ++i) {
            uint32_t bits;
            std::memcpy(&bits, &row[i], 4);
            h ^= bits;
            h *= 16777619u;
        }

        // Pick K experts
        for (int i = 0; i < k; ++i) {
            RouterToken tok;
            tok.row = b;
            tok.expert = (h + i) % experts; // Simple round-robin based on hash
            tok.weight = 1.0f / k; // Equal weight
            result.tokens.push_back(tok);
        }
    }
    return result;
}

RouterPlan routeSpatial(const RouterConfig &, const RouterPlan &basePlan, const cords::ViewR & )
{
    // Placeholder: Route everything to Expert 0 for now
    // Real implementation would involve KD-Tree or Octree lookups (adapters::FMM)
    RouterPlan result = basePlan;
    for (int b = 0; b < result.batchSize; ++b) {
        RouterToken tok;
        tok.row = b;
        tok.expert = 0;
        tok.weight = 1.0f;
        result.tokens.push_back(tok);
    }
    return result;
}

RouterPlan routeState(const RouterConfig &, const RouterPlan &basePlan, const cords::ViewR &)
{
    // Placeholder: Route based on simple state machine?
    // For now, round robin
    RouterPlan result = basePlan;
    int experts = result.numExperts;

    for (int b = 0; b < result.batchSize; ++b) {
        RouterToken tok;
        tok.row = b;
        tok.expert = b % experts;
        tok.weight = 1.0f;
        result.tokens.push_back(tok);
    }
    return result;
}

} // namespace job::ai::router
