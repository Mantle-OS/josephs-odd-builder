#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <deque>
#include <span>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "itoken.h"
#include "byte_lattice.h"

namespace job::ai::token {

class BPEToken final : public IToken {
public:
    static constexpr uint32_t kBaseVocab = 256;
    static constexpr uint32_t kInvalidId = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t kInvalidRank = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t kMaxLatticeId = (1u << 21) - 1u; // 7 bits x/y/z

    struct MergeRule {
        uint32_t left;
        uint32_t right;
        uint32_t newId;
    };

    struct Symbol {
        uint32_t prev           = kInvalidId;
        uint32_t next           = kInvalidId;
        uint32_t id             = 0;
    };

    // Inference queue element
    struct QueueElement {
        uint32_t pos;
        uint32_t rank;
        uint32_t leftId;
        uint32_t rightId;

        bool operator>(const QueueElement& o) const
        {
            if (rank != o.rank)
                return rank > o.rank;
            return pos > o.pos;
        }
    };

    BPEToken() = default;
    ~BPEToken() override = default;

    BPEToken(const BPEToken&) = default;
    BPEToken &operator=(const BPEToken&) = default;
    BPEToken(BPEToken&&) noexcept = default;
    BPEToken &operator=(BPEToken&&) noexcept = default;

    [[nodiscard]] TokenType type() const
    {
        return TokenType::BPE;
    }

    std::size_t encode(std::span<const uint8_t> input,
                       std::span<ByteLattice> output,
                       [[maybe_unused]] float mass) override
    {
        if (input.empty() || output.empty())
            return 0;

        if (m_pairToRank.empty()) {
            const size_t cnt = std::min(input.size(), output.size());
            for (size_t i = 0; i < cnt; ++i)
                output[i] = idToLattice(static_cast<uint32_t>(input[i]));
            return cnt;
        }

        const uint32_t n = static_cast<uint32_t>(input.size());
        std::vector<Symbol> syms(n);

        for (uint32_t i = 0; i < n; ++i) {
            syms[i].prev = (i == 0) ? kInvalidId : (i - 1);
            syms[i].next = (i + 1 == n) ? kInvalidId : (i + 1);
            syms[i].id   = static_cast<uint32_t>(input[i]);
        }

        std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<>> pq;

        auto pushPair = [&](uint32_t pos) {
            if (pos == kInvalidId) return;

            const uint32_t next = syms[pos].next;
            if (next == kInvalidId) return;

            const uint32_t leftId  = syms[pos].id;
            const uint32_t rightId = syms[next].id;

            uint32_t rank = kInvalidRank;

            // Fast-path for byte-byte pairs
            if (leftId < 256 && rightId < 256) {
                rank = m_bytePairRank[leftId * 256 + rightId];
            } else {
                const uint64_t key = (uint64_t(leftId) << 32) | rightId;
                auto it = m_pairToRank.find(key);
                if (it != m_pairToRank.end())
                    rank = it->second;
            }

            if (rank != kInvalidRank) {
                pq.push({pos, rank, leftId, rightId});
            }
        };

        for (uint32_t i = 0; i + 1 < n; ++i)
            pushPair(i);

        while (!pq.empty()) {
            const auto top = pq.top();
            pq.pop();

            if (syms[top.pos].id != top.leftId)
                continue;

            const uint32_t next = syms[top.pos].next;
            if (next == kInvalidId || syms[next].id != top.rightId)
                continue;

            if (top.rank >= m_rules.size())
                continue;

            // Re-check that this pair still maps to the same rank
            const uint64_t key = (uint64_t(top.leftId) << 32) | top.rightId;
            auto it = m_pairToRank.find(key);
            if (it == m_pairToRank.end() || it->second != top.rank)
                continue;

            const uint32_t newId = m_rules[top.rank].newId;
            syms[top.pos].id = newId;

            const uint32_t nextNext = syms[next].next;
            syms[top.pos].next = nextNext;
            if (nextNext != kInvalidId)
                syms[nextNext].prev = top.pos;

            pushPair(syms[top.pos].prev);
            pushPair(top.pos);
        }


        size_t outCnt = 0;
        uint32_t cur = 0;
        while (cur != kInvalidId && outCnt < output.size()) {
            output[outCnt++] = idToLattice(syms[cur].id);
            cur = syms[cur].next;
        }
        return outCnt;
    }

    std::size_t decode(std::span<const ByteLattice> input, std::span<uint8_t> output) override
    {
        std::vector<uint8_t> buf;
        buf.reserve(input.size() * 2);

        for (const auto &lattice : input)
            expandIterative(latticeToId(lattice), buf);

        const size_t cnt = std::min(buf.size(), output.size());
        std::copy_n(buf.begin(), cnt, output.begin());
        return cnt;
    }


    bool save(std::ostream &os) const override
    {
        if (!IToken::save(os))
            return false;

        const uint32_t magic = 0xFEEDBEEF;
        const uint32_t ver   = 0;
        os.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        os.write(reinterpret_cast<const char*>(&ver), sizeof(ver));

        const uint32_t count = static_cast<uint32_t>(m_rules.size());
        os.write(reinterpret_cast<const char*>(&count), sizeof(count));

        if (count)
            os.write(reinterpret_cast<const char*>(m_rules.data()), static_cast<std::streamsize>(count * sizeof(MergeRule)));

        return os.good();
    }

    bool load(std::istream &is) override
    {
        uint32_t magic = 0;
        uint32_t ver = 0;
        if (!is.read(reinterpret_cast<char*>(&magic), sizeof(magic)))
            return false;

        if (magic != 0xFEEDBEEF)
            return false;

        if (!is.read(reinterpret_cast<char*>(&ver), sizeof(ver)))
            return false;

        uint32_t count = 0;
        if (!is.read(reinterpret_cast<char*>(&count), sizeof(count)))
            return false;

        m_rules.resize(count);
        if (count)
            if (!is.read(reinterpret_cast<char*>(m_rules.data()), static_cast<std::streamsize>(count * sizeof(MergeRule))))
                return false;

        rebuildAccelerators();
        return true;
    }



    void train(const std::string& corpus, uint32_t targetVocabSize)
    {

        m_rules.clear();
        m_pairToRank.clear();
        if (corpus.empty() || targetVocabSize <= kBaseVocab)
            return;

        if (targetVocabSize > kBaseVocab + kMaxLatticeId)
            targetVocabSize = kBaseVocab + kMaxLatticeId;


        const uint32_t n = (uint32_t)corpus.size();
        std::vector<Symbol> syms(n);
        for (uint32_t i = 0; i < n; ++i) {
            syms[i].id   = (uint8_t)corpus[i];
            syms[i].prev = (i == 0) ? kInvalidId : (i - 1);
            syms[i].next = (i + 1 == n) ? kInvalidId : (i + 1);
        }

        auto makeKey = [&](uint32_t l, uint32_t r) -> uint64_t {
            return (uint64_t(l) << 32) | uint64_t(r);
        };

        std::unordered_map<uint64_t, int32_t> counts;
        std::unordered_map<uint64_t, std::deque<uint32_t>> occ;

        struct TrainPQ {
            int32_t count;
            uint64_t key;
            // max-heap by count, tie-break by smaller key
            bool operator<(const TrainPQ& o) const
            {
                if (count != o.count)
                    return count < o.count;
                return key > o.key;
            }
        };
        std::priority_queue<TrainPQ> pq;

        auto inc = [&](uint64_t key, uint32_t pos) {
            occ[key].push_back(pos);
            int32_t v = ++counts[key];
            pq.push({v, key});
        };

        auto dec = [&](uint64_t key) {
            auto it = counts.find(key);
            if (it == counts.end())
                return;

            if (--it->second <= 0)
                counts.erase(it);
        };

        auto pairAt = [&](uint32_t pos, uint32_t& l, uint32_t& r) -> bool {
            if (pos == kInvalidId)
                return false;

            uint32_t nx = syms[pos].next;
            if (nx == kInvalidId)
                return false;

            l = syms[pos].id;
            r = syms[nx].id;

            return true;
        };

        for (uint32_t i = 0; i + 1 < n; ++i) {
            uint32_t l, r;
            if (pairAt(i, l, r))
                inc(makeKey(l, r), i);
        }

        const uint32_t maxMerges = targetVocabSize - kBaseVocab;

        while (m_rules.size() < maxMerges && !pq.empty()) {
            auto top = pq.top(); pq.pop();

            auto itc = counts.find(top.key);
            if (itc == counts.end() || itc->second != top.count) continue;

            const uint32_t leftId  = uint32_t(top.key >> 32);
            const uint32_t rightId = uint32_t(top.key & 0xFFFFFFFFu);

            const uint32_t rank  = (uint32_t)m_rules.size();
            const uint32_t newId = kBaseVocab + rank;

            m_rules.push_back({leftId, rightId, newId});
            m_pairToRank[top.key] = rank;

            auto itOcc = occ.find(top.key);
            if (itOcc == occ.end()) {
                counts.erase(top.key);
                continue;
            }

            auto &dq = itOcc->second;

            // Consume all occurrences of this key (lazy skip stale)
            while (!dq.empty()) {
                uint32_t pos = dq.front();
                dq.pop_front();

                // Validate it still matches
                uint32_t nx = syms[pos].next;
                if (nx == kInvalidId)
                    continue;

                if (syms[pos].id != leftId)
                    continue;

                if (syms[nx].id != rightId)
                    continue;

                // Neighbors before merge
                uint32_t pv = syms[pos].prev;
                uint32_t nn = syms[nx].next;

                // decrement old adjacent pairs: (pv,pos) and (nx,nn)
                if (pv != kInvalidId) {
                    dec(makeKey(syms[pv].id, leftId));
                }
                if (nn != kInvalidId) {
                    dec(makeKey(rightId, syms[nn].id));
                }

                // decrement this pair occurrence
                dec(top.key);

                // Apply merge: pos becomes newId, nx removed from chain
                syms[pos].id = newId;
                syms[pos].next = nn;
                if (nn != kInvalidId)
                    syms[nn].prev = pos;

                // increment new adjacent pairs: (pv,new) and (new,nn)
                if (pv != kInvalidId)
                    inc(makeKey(syms[pv].id, newId), pv);
                if (nn != kInvalidId)
                    inc(makeKey(newId, syms[nn].id), pos);
            }

            // cleanup key (may already be erased by dec to 0)
            occ.erase(top.key);
            counts.erase(top.key);
        }

        rebuildAccelerators();
    }



private:
    void rebuildAccelerators()
    {
        m_pairToRank.clear();
        m_pairToRank.reserve(m_rules.size() * 2);

        m_bytePairRank.fill(kInvalidRank);

        for (uint32_t rank = 0; rank < m_rules.size(); ++rank) {
            const auto& r = m_rules[rank];
            const uint64_t key = (uint64_t(r.left) << 32) | r.right;
            m_pairToRank[key] = rank;

            if (r.left < 256 && r.right < 256) {
                m_bytePairRank[r.left * 256 + r.right] = rank;
            }
        }
    }
    void expandIterative(uint32_t id, std::vector<uint8_t> &out) const
    {
        std::vector<uint32_t> st;
        st.push_back(id);
        while (!st.empty()) {
            uint32_t cur = st.back();
            st.pop_back();
            if (cur < kBaseVocab) {
                out.push_back(static_cast<uint8_t>(cur));
            } else {
                uint32_t rank = cur - kBaseVocab;
                if (rank < m_rules.size()) {
                    st.push_back(m_rules[rank].right);
                    st.push_back(m_rules[rank].left);
                }
            }
        }
    }

    static constexpr float kInv64 = 0.015625f;
    // static constexpr uint32_t kMaxLatticeId = (1u << 21) - 1u;

    ByteLattice idToLattice(uint32_t id) const noexcept
    {
        const uint32_t c0 =  id        & 0x7Fu;
        const uint32_t c1 = (id >> 7)  & 0x7Fu;
        const uint32_t c2 = (id >> 14) & 0x7Fu;
        return {
            float(c0) * kInv64 - 1.0f,
            float(c1) * kInv64 - 1.0f,
            float(c2) * kInv64 - 1.0f,
            1.0f
        };
    }

    uint32_t latticeToId(const ByteLattice& p) const noexcept
    {
        int i0 = std::clamp((int)std::lrintf((p.x + 1.0f) * 64.0f), 0, 127);
        int i1 = std::clamp((int)std::lrintf((p.y + 1.0f) * 64.0f), 0, 127);
        int i2 = std::clamp((int)std::lrintf((p.z + 1.0f) * 64.0f), 0, 127);
        return uint32_t(i0) | (uint32_t(i1) << 7) | (uint32_t(i2) << 14);
    }


    std::array<uint32_t, 256 * 256>             m_bytePairRank{}; // (leftByte*256 + rightByte) -> rank or kInvalidRank
    std::vector<MergeRule>                      m_rules;
    std::unordered_map<uint64_t, uint32_t>      m_pairToRank;
};

} // namespace job::ai::token



