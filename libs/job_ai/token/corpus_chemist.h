#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include <split_mix64.h>
#include <job_monte_carlo.h>

namespace job::ai::token {

class CorpusChemist {
public:
    explicit CorpusChemist(std::string_view corpus, threads::ThreadPool::Ptr pool = nullptr) :
        m_corpus(corpus),
        m_pool(std::move(pool))
    {
        if(!m_pool)
            JOB_LOG_WARN("[The Chemist] has no pool to swim in. This is not good.");
    }

    [[nodiscard]] std::string findReactiveMolecule(int len, int samples, uint64_t seed)
    {
        if (len <= 0 || samples <= 0)
            return "";

        const std::size_t n = m_corpus.size();
        const std::size_t L = static_cast<std::size_t>(len);

        if (n < L)
            return "";

        // NOTE: We sample positions in [0, n - L].
        const std::size_t maxPos = n - L;

        // Small bounded TopK result from each block.
        constexpr std::size_t kTopK = 32;

        struct Candidate {
            uint64_t key = 0;
            uint32_t count = 0;
            uint32_t pos = 0; // representative position for reconstruction
        };

        struct TopK {
            std::array<Candidate, kTopK> items{};
            std::size_t used = 0;

            void push(uint64_t key, uint32_t count, uint32_t pos)
            {
                if (used < kTopK) {
                    items[used++] = { key, count, pos };
                    return;
                }

                // replace worst
                std::size_t worst = 0;
                for (std::size_t i = 1; i < used; ++i)
                    if (items[i].count < items[worst].count)
                        worst = i;

                if (count > items[worst].count)
                    items[worst] = { key, count, pos };
            }

            void merge(const TopK &other)
            {
                for (std::size_t i = 0; i < other.used; ++i)
                    push(other.items[i].key, other.items[i].count, other.items[i].pos);
            }
        };

        // 64-bit fingerprint of fragment (byte string).
        // This is intentionally simple and deterministic. You can swap it later.
        auto hashFragment = [&](std::size_t pos) -> uint64_t {
            const uint8_t *p = reinterpret_cast<const uint8_t*>(m_corpus.data() + pos);
            uint64_t h = 0x9E3779B97F4A7C15ull ^ static_cast<uint64_t>(L);
            for (std::size_t i = 0; i < L; ++i)
                h ^= (uint64_t)p[i] + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);

            // tag length in top byte so different lens don't collide as often
            h ^= (uint64_t(L) & 0xFFull) << 56;
            return h;
        };

        if (!m_pool)
            return findReactiveMoleculeSingleThread(len, samples, seed); // fallback path

        threads::MonteCarloEngine<float> mc(m_pool);

        // sampleReduce: accumulator is TopK
        TopK top = mc.sampleReduce<TopK>(
            static_cast<std::size_t>(samples),
            TopK{},
            [&](job::core::SplitMix64 &rng, std::size_t /*i*/, TopK &acc) {
                static thread_local std::unordered_map<uint64_t, uint32_t> local;
                local.clear();
                local.reserve(256);

                constexpr int kInner = 8;

                // Use one draw immediately (so 'pos' isn't dead), then do the remaining kInner-1.
                const std::size_t p0 = static_cast<std::size_t>(
                    rng.rangeIndex(static_cast<uint64_t>(maxPos) + 1)
                    );

                {
                    const uint64_t key = hashFragment(p0);
                    const uint32_t c = ++local[key];
                    acc.push(key, c, static_cast<uint32_t>(p0));
                }

                for (int t = 1; t < kInner; ++t) {
                    const std::size_t p2 = static_cast<std::size_t>(
                        rng.rangeIndex(static_cast<uint64_t>(maxPos) + 1)
                        );

                    const uint64_t key = hashFragment(p2);
                    const uint32_t c = ++local[key];
                    acc.push(key, c, static_cast<uint32_t>(p2));
                }
            },
            [](TopK a, const TopK &b) {
                a.merge(b);
                return a;
            },
            seed,
            /*priority=*/0,
            /*grain=*/4096);

        // Pick best candidate from merged TopK
        Candidate best{};
        for (std::size_t i = 0; i < top.used; ++i) {
            if (top.items[i].count > best.count)
                best = top.items[i];
        }

        if (best.count <= 1)
            return "";

        const std::size_t pos = std::min<std::size_t>(best.pos, maxPos);
        return std::string(m_corpus.substr(pos, L));
    }

private:
    // A safe fallback if pool isn't wired yet.
    [[nodiscard]] std::string findReactiveMoleculeSingleThread(int len, int samples, uint64_t seed)
    {
        const std::size_t n = m_corpus.size();
        const std::size_t L = static_cast<std::size_t>(len);
        if (n < L)
            return "";

        const std::size_t maxPos = n - L;

        core::SplitMix64 rng(seed);

        // Fingerprint -> count, and keep a representative position.
        std::unordered_map<uint64_t, std::pair<uint32_t, uint32_t>> counts;
        counts.reserve(static_cast<std::size_t>(samples));

        auto hashFragment = [&](std::size_t pos) -> uint64_t {
            const uint8_t *p = reinterpret_cast<const uint8_t*>(m_corpus.data() + pos);
            uint64_t h = 0x9E3779B97F4A7C15ull ^ static_cast<uint64_t>(L);
            for (std::size_t i = 0; i < L; ++i) {
                h ^= (uint64_t)p[i] + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
            }
            h ^= (uint64_t(L) & 0xFFull) << 56;
            return h;
        };

        [[maybe_unused]] uint64_t bestKey = 0;
        uint32_t bestCount = 0;
        uint32_t bestPos = 0;

        for (int i = 0; i < samples; ++i) {
            const std::size_t pos = static_cast<std::size_t>(rng.rangeIndex(static_cast<uint64_t>(maxPos) + 1));
            const uint64_t key = hashFragment(pos);

            auto &entry = counts[key];
            entry.second = static_cast<uint32_t>(pos); // keep last seen pos
            const uint32_t c = ++entry.first;

            if (c > bestCount) {
                bestCount = c;
                bestKey = key;
                bestPos = static_cast<uint32_t>(pos);
            }
        }

        if (bestCount <= 1)
            return "";

        const std::size_t pos = std::min<std::size_t>(bestPos, maxPos);
        return std::string(m_corpus.substr(pos, L));
    }

private:
    std::string_view                m_corpus;
    threads::ThreadPool::Ptr        m_pool;
};

} // namespace job::ai::token
