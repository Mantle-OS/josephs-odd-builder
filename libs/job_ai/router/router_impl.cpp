#include "router_impl.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstring>

namespace job::ai::router {

namespace {

constexpr int kMaxK = 16;
struct ValIdx {
    float val;
    int id;
};
struct MinHeapComparator {
    bool operator()(const ValIdx &a, const ValIdx &b) const noexcept
    {
        return a.val > b.val;
    }
};

// Helper: Find Top-K (Stack based, zero alloc)
// rename n and k to something sane
void findTopK_Stack(const float *row, int n, int k, ValIdx *out_buffer)
{

    assert(row);
    assert(out_buffer);
    assert(n > 0);
    assert(k > 0);
    assert(k <= n);

    for (int i = 0; i < k; ++i)
        out_buffer[i] = {row[i], i};

    std::make_heap(out_buffer, out_buffer + k, MinHeapComparator{});

    for (int i = k; i < n; ++i) {
        if (row[i] > out_buffer[0].val) {
            std::pop_heap(out_buffer, out_buffer + k, [](auto &a, auto &b){
                return a.val > b.val;
            });
            out_buffer[k-1] = {row[i], i};
            std::push_heap(out_buffer, out_buffer + k, [](auto &a, auto &b){
                return a.val > b.val;
            });
        }
    }
    std::sort(out_buffer, out_buffer + k, [](auto& a, auto& b){
        return a.val > b.val;
    });
}

adapters::AdapterType getAdapterForExpert(const RouterConfig &cfg, int expertId)
{
    if (static_cast<size_t>(expertId) < cfg.experts.size())
        return cfg.experts[expertId].adapter;
    return adapters::AdapterType::Dense;
}

} // namespace

RouterPlan routeTopK(const RouterConfig &cfg, int batch, int experts, const float *logits, RouterToken *buffer)
{    
    int k = cfg.topK;
    if (k > kMaxK)
        k = kMaxK;
    size_t tokenCount = 0;
    ValIdx best[kMaxK];

    for (int b = 0; b < batch; ++b) {
        const float *rowLogits = logits + b * experts;
        findTopK_Stack(rowLogits, experts, k, best);

        float maxVal = best[0].val;
        float sumExp = 0.0f;
        for (int i = 0; i < k; ++i) {
            best[i].val = std::exp(best[i].val - maxVal);
            sumExp += best[i].val;
        }

        float invSum = 1.0f / (sumExp + 1e-9f);

        for (int i = 0; i < k; ++i) {
            RouterToken &tok = buffer[tokenCount++];
            tok.row = b;
            tok.expert = best[i].id;
            tok.weight = best[i].val * invSum;
            tok.adapter = getAdapterForExpert(cfg, tok.expert);
        }
    }
    // matching new struct order: batchSize, numExperts, tokens, tokenCount
    return RouterPlan{ batch, experts, buffer, tokenCount };
}

RouterPlan routeHash(const RouterConfig &cfg, int batch, int experts, const cords::ViewR &input, RouterToken *buffer)
{
    int k = cfg.topK;
    size_t tokenCount = 0;
    const float *data = input.data();
    int dim = input.extent()[1];
    int stride = std::max(1, dim / 8);
    int scanCount = std::min(8, dim);

    for (int b = 0; b < batch; ++b) {
        uint32_t h = 2166136261u;
        const float *row = data + b * dim;

        for(int i = 0; i < scanCount; ++i) {
            uint32_t bits;
            std::memcpy(&bits, &row[i * stride], 4);
            h ^= bits;
            h *= 16777619u;
        }

        for (int i = 0; i < k; ++i) {
            RouterToken &tok = buffer[tokenCount++];
            tok.row = b;
            tok.expert = (h + i) % experts;
            tok.weight = 1.0f / k;
            tok.adapter = getAdapterForExpert(cfg, tok.expert);
        }
    }
    // matching new struct order
    return RouterPlan{ batch, experts, buffer, tokenCount };
}

RouterPlan routeSpatial(const RouterConfig &cfg, int batch, [[maybe_unused]] const cords::ViewR &input, RouterToken *buffer)
{
    size_t tokenCount = 0;
    int k = cfg.topK;
    // Total experts unknown from input, assume from config or just 1 for this stub
    int experts = static_cast<int>(cfg.experts.size());
    if (experts == 0)
        experts = 1;

    for (int b = 0; b < batch; ++b) {
        for (int i = 0; i < k; ++i) {
            RouterToken &tok = buffer[tokenCount++];
            tok.row = b;
            tok.expert = i % experts;
            tok.weight = 1.0f / k;
            tok.adapter = getAdapterForExpert(cfg, tok.expert);
        }
    }
    // matching new struct order
    return RouterPlan{ batch, experts, buffer, tokenCount };
}

RouterPlan routeState(const RouterConfig &cfg, int batch, int experts, RouterToken *buffer)
{
    size_t tokenCount = 0;
    int k = cfg.topK;

    for (int b = 0; b < batch; ++b) {
        int baseExpert = b % experts;
        for (int i = 0; i < k; ++i) {
            RouterToken &tok = buffer[tokenCount++];
            tok.row = b;
            tok.expert = (baseExpert + i) % experts;
            tok.weight = 1.0f / k;
            tok.adapter = getAdapterForExpert(cfg, tok.expert);
        }
    }
    // matching new struct order
    return RouterPlan{ batch, experts, buffer, tokenCount };
}

} // namespace job::ai::router
