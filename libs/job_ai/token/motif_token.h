#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <span>
#include <cmath>

#include <job_logger.h>
#include <split_mix64.h>
#include <utils/utf8_decoder.h>

#include "itoken.h"
#include "byte_lattice.h"
#include "byte_lattice_kernel.h"
#include "corpus_chemist.h"

namespace job::ai::token {

enum class DecayType : uint8_t {
    FastDecay   = 0,    // Half-life is short (>>= 1)
    SlowDecay   = 1,    // Gradual decay (* 0.9)
    Stable      = 2     // No decay
};

class MotifToken final : public IToken {
public:
    static constexpr uint32_t kMaxCapacity   = 16384; // 128x128 plane
    static constexpr uint32_t kImmutableZone = 100;

    MotifToken()
    {
        // "God Particles" (immutable-ish starter motifs)
        addMotif("The ");
        addMotif(" the ");
        addMotif(" of ");
        addMotif(" and ");
        addMotif(" to ");
        addMotif(" in ");
        addMotif("ing");
        addMotif("tion");
        addMotif("Job");  // Self-awareness :>)
    }

    // Bytes -> atoms.
    // Input is assumed normalized UTF-8 bytes (Policy 1).
    [[nodiscard]] std::size_t encode(std::span<const uint8_t> input,
                                     std::span<ByteLattice> output,
                                     float mass = 1.0f) override
    {
        std::size_t outCount = 0;
        std::size_t i = 0;

        // Greedy scan (bounded)
        constexpr std::size_t kMaxGreedyScan = 32;

        while (i < input.size() && outCount < output.size()) {
            bool foundMotif = false;

            const std::size_t remain = input.size() - i;
            const std::size_t maxScan = std::min<std::size_t>({ m_maxMotifLen, remain, kMaxGreedyScan });

            if (m_minMotifLen != 0 && m_minMotifLen <= m_maxMotifLen && maxScan >= m_minMotifLen) {
                for (std::size_t scanLen = maxScan;; --scanLen) {
                    // NOTE: string_view is over bytes. This is intentional in byte-world.
                    const std::string_view sub(reinterpret_cast<const char*>(input.data() + i), scanLen);

                    auto it = m_strToId.find(sub);
                    if (it != m_strToId.end()) {
                        const uint32_t id = it->second;

                        // usage accounting
                        ++m_motifUsage[id];

                        // emit motif atom
                        output[outCount++] = idToLattice(id, mass);

                        i += scanLen;
                        foundMotif = true;
                        break;
                    }

                    if (scanLen == m_minMotifLen)
                        break; // prevent unsigned wrap
                }
            }

            if (foundMotif)
                continue;

            // Fallback: one byte -> one atom
            output[outCount++] = ByteLattice::encode(input[i], mass);
            ++i;
        }

        return outCount;
    }

    // Atoms -> bytes.
    // Returns bytes written. Expansions happen for motifs.
    [[nodiscard]] std::size_t decode(std::span<const ByteLattice> input,
                                     std::span<uint8_t> output) override
    {
        std::size_t outCount = 0;

        for (std::size_t i = 0; i < input.size(); ++i) {
            if (outCount >= output.size())
                break;

            const ByteLattice &p = input[i];

            if (isMotifLane(p)) {
                const uint32_t id = latticeToId(p);

                auto it = m_idToStr.find(id);
                if (it == m_idToStr.end()) {
                    // Unknown motif: emit U+FFFD (EF BF BD) if space, else stop.
                    static constexpr uint8_t kFFFD[3] = { 0xEF, 0xBF, 0xBD };
                    if (outCount + 3 > output.size())
                        break;

                    output[outCount++] = kFFFD[0];
                    output[outCount++] = kFFFD[1];
                    output[outCount++] = kFFFD[2];
                    continue;
                }

                const std::string &s = it->second;
                if (outCount + s.size() > output.size())
                    break;

                // Copy motif bytes out
                std::copy(reinterpret_cast<const uint8_t*>(s.data()),
                          reinterpret_cast<const uint8_t*>(s.data()) + s.size(),
                          output.data() + outCount);

                outCount += s.size();
            } else {
                // Raw byte atom
                output[outCount++] = ByteLattice::decode(p);
            }
        }

        return outCount;
    }

    void mutate(uint64_t seed) override
    {
        if (!m_chemist)
            return;

        lookForSquatters();

        job::core::SplitMix64 rng(seed);

        int bondLen = 2;
        const float r = rng.nextFloat();
        if (r < 0.50f)
            bondLen = 2;
        else if (r < 0.80f)
            bondLen = 3;
        else if (r < 0.95f)
            bondLen = 4;
        else
            bondLen = 5;

        // NOTE: chemist currently samples bytes, not codepoints.
        // Under Policy 1 the corpus should also be normalized UTF-8 bytes.
        const std::string newMolecule = m_chemist->findReactiveMolecule(bondLen, 1000, seed);

        // If corpus is normalized UTF-8, this should usually be true for aligned sequences.
        if (!newMolecule.empty() && ansi::utils::Utf8Decoder::isValid(newMolecule))
            addMotif(newMolecule);
    }

    // NOTE: CorpusChemist stores string_view; caller must keep corpus storage alive.
    void setCorpus(std::string_view corpus, threads::ThreadPool::Ptr pool = nullptr)
    {
        m_chemist = std::make_unique<CorpusChemist>(corpus, pool);
    }

    void setDecayType(DecayType decayType) noexcept { m_decayType = decayType; }
    [[nodiscard]] DecayType decayType() const noexcept { return m_decayType; }
    [[nodiscard]] std::size_t getMotifCount() const { return m_strToId.size(); }
    [[nodiscard]] std::size_t getMaxMotifLength() const noexcept { return m_maxMotifLen; }

    void debug() const
    {
        JOB_LOG_INFO("MotifToken Statistics");
        JOB_LOG_INFO("  Vocabulary: {}/{}", m_strToId.size(), kMaxCapacity);
        JOB_LOG_INFO("  Free IDs: {}", m_freeIds.size());
        JOB_LOG_INFO("  Length range: [{}, {}]", m_minMotifLen, m_maxMotifLen);
        JOB_LOG_INFO("  Decay mode: {}",
                     m_decayType == DecayType::FastDecay ? "FastDecay" :
                         m_decayType == DecayType::SlowDecay ? "SlowDecay" : "Stable");

        std::vector<std::pair<std::string, uint32_t>> top;
        top.reserve(m_idToStr.size());

        for (const auto &[id, str] : m_idToStr) {
            auto usageIt = m_motifUsage.find(id);
            const uint32_t usage = (usageIt != m_motifUsage.end()) ? usageIt->second : 0;
            top.push_back({str, usage});
        }

        const std::size_t topN = std::min<std::size_t>(10, top.size());
        if (topN == 0) {
            JOB_LOG_INFO("  No motifs to display");
            return;
        }

        std::partial_sort(top.begin(), top.begin() + topN, top.end(),
                          [](const auto &a, const auto &b) { return a.second > b.second; });

        JOB_LOG_INFO("  Top {} motifs:", topN);
        for (std::size_t i = 0; i < topN; ++i) {
            JOB_LOG_INFO("    {}. \"{}\" ({} uses)", i + 1, top[i].first, top[i].second);
        }
    }

private:
    [[nodiscard]] static constexpr bool isMotifLane(const ByteLattice &p) noexcept
    {
        return p.z > 0.5f;
    }

    ByteLattice idToLattice(uint32_t id, float mass = 1.0f) const
    {
        const float x = (float)(id & 0x7F);         // low 7
        const float y = (float)((id >> 7) & 0x7F);  // high 7

        return {
            (x * 0.015625f) - 1.0f,
            (y * 0.015625f) - 1.0f,
            1.0f, // motif lane
            mass
        };
    }

    uint32_t latticeToId(const ByteLattice &p) const
    {
        // Snap to grid like ByteLattice decode does.
        const int i_x = std::clamp((int)std::lrintf((p.x + 1.0f) * 64.0f), 0, 127);
        const int i_y = std::clamp((int)std::lrintf((p.y + 1.0f) * 64.0f), 0, 127);
        return (uint32_t)(i_x | (i_y << 7));
    }


    void lookForSquatters()
    {
        if (m_opCount++ % 1000 == 0)
            removeSquatters();
    }

    void removeSquatters()
    {
        if (m_idToStr.size() <= kImmutableZone)
            return;

        std::vector<std::pair<uint32_t, uint32_t>> usageList;
        usageList.reserve(m_idToStr.size());

        for (const auto &[id, str] : m_idToStr) {
            if (id < kImmutableZone)
                continue;
            usageList.push_back({ id, m_motifUsage[id] });
        }

        const std::size_t numToEvaporate = usageList.size() / 10;
        if (numToEvaporate == 0)
            return;

        std::nth_element(usageList.begin(),
                         usageList.begin() + numToEvaporate,
                         usageList.end(),
                         [](const auto &a, const auto &b) { return a.second < b.second; });

        for (std::size_t i = 0; i < numToEvaporate; ++i) {
            const uint32_t victimId = usageList[i].first;

            auto it = m_idToStr.find(victimId);
            if (it != m_idToStr.end()) {
                m_strToId.erase(it->second);
                m_idToStr.erase(it);
                m_motifUsage.erase(victimId);
                m_freeIds.push_back(victimId);
            }
        }

        for (auto &[id, count] : m_motifUsage) {
            switch (m_decayType) {
            case DecayType::FastDecay:
                count >>= 1;
                break;
            case DecayType::SlowDecay:
                count = (count * 9) / 10;
                break;
            case DecayType::Stable:
            default: break;
            }
        }
    }

    uint32_t decay()
    {
        if (m_nextId < kMaxCapacity)
            return m_nextId++;

        uint32_t victimId = kImmutableZone;
        uint32_t minUsage = UINT32_MAX;

        for (const auto &[id, str] : m_idToStr) {
            if (id < kImmutableZone)
                continue;

            const uint32_t usage = m_motifUsage[id];
            if (usage < minUsage) {
                minUsage = usage;
                victimId = id;
            }
        }

        auto it = m_idToStr.find(victimId);
        if (it != m_idToStr.end()) {
            m_strToId.erase(it->second);
            m_idToStr.erase(it);
            m_motifUsage.erase(victimId);
        }

        return victimId;
    }

    void addMotif(std::string_view sv)
    {
        if (sv.empty())
            return;

        if (m_strToId.find(sv) != m_strToId.end())
            return;

        uint32_t id = 0;
        if (!m_freeIds.empty()) {
            id = m_freeIds.back();
            m_freeIds.pop_back();
        } else if (m_nextId < kMaxCapacity) {
            id = m_nextId++;
        } else {
            id = decay();
        }

        auto [it, inserted] = m_strToId.try_emplace(std::string(sv), id);
        if (!inserted)
            return;

        m_idToStr[id] = it->first;

        const std::size_t len = it->first.size();
        m_maxMotifLen = std::max(m_maxMotifLen, len);
        if (m_minMotifLen == 0)
            m_minMotifLen = len;
        else
            m_minMotifLen = std::min(m_minMotifLen, len);
    }

    struct StringHash {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(std::string_view sv) const
        {
            return std::hash<std::string_view>{}(sv);
        }

        [[nodiscard]] size_t operator()(const std::string &s) const
        {
            return std::hash<std::string>{}(s);
        }
    };

    struct StringEqual {
        using is_transparent = void;
        [[nodiscard]] bool operator()(std::string_view lhs, std::string_view rhs) const
        {
            return lhs == rhs;
        }
    };

private:
    // NOTE: kept as-is for now (no behavior loss). We can later collapse m_idToStr to string_view to avoid double storage.
    std::unordered_map<std::string, uint32_t, StringHash, StringEqual>  m_strToId;
    std::unordered_map<uint32_t, std::string>                           m_idToStr;
    std::unordered_map<uint32_t, uint32_t>                              m_motifUsage;

    std::size_t                                                         m_opCount = 0;

    uint32_t                                                            m_nextId = 0;
    std::size_t                                                         m_maxMotifLen = 0;
    std::size_t                                                         m_minMotifLen = 0;

    std::unique_ptr<CorpusChemist>                                      m_chemist;
    std::vector<uint32_t>                                               m_freeIds;

    DecayType                                                           m_decayType = DecayType::FastDecay;
};

} // namespace job::ai::token
