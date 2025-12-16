#pragma once
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <memory>

#include <job_logger.h>
#include <split_mix64.h>

#include <utils/utf8_decoder.h>

#include "itoken.h"

#include "corpus_chemist.h"

namespace job::ai::token {

// We could have fun with the names here.
enum class DecayType : uint8_t {
    FastDecay   = 0,    // Half-life is short (>>= 1)
    SlowDecay   = 1,    // Gradual decay (* 0.9)
    Stable      = 2     // No decay
};

class MotifToken final : public IToken {
public:
    // The Motif Plane is 128x128.
    // We reserve the first 100 slots for "The God Particles" (Immutable Core Vocab)
    static constexpr uint32_t kMaxCapacity   = 16384;
    static constexpr uint32_t kImmutableZone = 100;
    MotifToken() :
        m_rng(12345)
    {
        // Sorted by frequency in English (more common = checked first)
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

    [[nodiscard]] std::vector<UnicodeLattice> encode(std::string_view text, float mass = 1.0f) override
    {
        std::vector<UnicodeLattice> particles;
        particles.reserve(text.size() / 2); // Heuristic: assume some compression
        size_t i = 0;
        const size_t len = text.size();

        while (i < len) {
            bool foundMotif = false;

            constexpr size_t kMaxGreedyScan = 32;
            const size_t maxScan = std::min({ m_maxMotifLen, len - i, kMaxGreedyScan });
            if (m_minMotifLen != 0 && m_minMotifLen <= m_maxMotifLen && maxScan >= m_minMotifLen ) {


                // Inclusive scan: maxScan down to m_minMotifLen
                for (size_t scanLen = maxScan; ; --scanLen) {
                    std::string_view sub = text.substr(i, scanLen);

                    auto it = m_strToId.find(sub);
                    if (it != m_strToId.end()) {
                        m_motifUsage[it->second]++;
                        particles.push_back(idToLattice(it->second, mass));
                        i += scanLen;
                        foundMotif = true;
                        break;
                    }

                    if (scanLen == m_minMotifLen)
                        break; // prevent unsigned wrap and ensure we tested min len
                }
            }

            if (foundMotif)
                continue;

            // Fallback: encode single UTF-8 atom
            const unsigned char lead = static_cast<unsigned char>(text[i]);
            const size_t charLen = ansi::utils::Utf8Decoder::determineLength(lead);

            if (charLen == 0 || i + charLen > len) {
                particles.push_back(UnicodeLattice::encode(0xFFFD, mass));
                i++;
                continue;
            }

            const std::string_view atomChunk = text.substr(i, charLen);
            const char32_t atom = ansi::utils::Utf8Decoder::decodeUtf8(atomChunk);
            particles.push_back(UnicodeLattice::encode(atom, mass));
            i += charLen;
        }

        return particles;
    }

    [[nodiscard]] std::string decode(const std::vector<UnicodeLattice> &tokens) override
    {
        std::string result;
        for (const auto &token : tokens) {
            if (token.z >= 0.85f && token.z <= 1.0f) {
                // Motif
                const uint32_t id = latticeToId(token);
                if (auto it = m_idToStr.find(id); it != m_idToStr.end())
                    result += it->second;
                else
                    result += "�"; // U+FFFD replacement character

            } else {
                // Atom (fallback)
                char32_t c = UnicodeLattice::decode(token);
                ansi::utils::Utf8Decoder::encodeTo(c, result);
            }
        }
        return result;
    }

    void setCorpus(std::string_view corpus)
    {
        m_chemist = std::make_unique<CorpusChemist>(corpus);
    }

    void mutate(uint64_t seed) override
    {
        if (!m_chemist)
            return;

        lookForSquatters();

        job::core::SplitMix64 rng(seed);
        int bondLen = 2;
        float r = rng.nextFloat();
        if (r < 0.50f)
            bondLen = 2;
        else if (r < 0.80f)
            bondLen = 3;
        else if (r < 0.95f)
            bondLen = 4;
        else
            bondLen = 5;

        // Monte Carlo sampling to find frequent n-grams
        const std::string newMolecule = m_chemist->findReactiveMolecule(bondLen, 1000, seed);

        if (!newMolecule.empty() && ansi::utils::Utf8Decoder::isValid(newMolecule))
            addMotif(newMolecule);
    }

    // "Squatter's rights" are not recognized in this memory bank!
    void lookForSquatters()
    {
        if (m_opCount++ % 1000 == 0)
            removeSquatters();
    }

    [[nodiscard]] size_t getMotifCount() const
    {
        return m_strToId.size();
    }

    [[nodiscard]] size_t getMaxMotifLength() const noexcept
    {
        return m_maxMotifLen;
    }

    void setDecayType(DecayType decayType) noexcept
    {
        m_decayType = decayType;
    }

    [[nodiscard]] DecayType decayType() const noexcept
    {
        return m_decayType;
    }

    void debug() const
    {
        JOB_LOG_INFO("MotifToken Statistics");
        JOB_LOG_INFO("  Vocabulary: {}/{}", m_strToId.size(), kMaxCapacity);
        JOB_LOG_INFO("  Free IDs: {}", m_freeIds.size());
        JOB_LOG_INFO("  Length range: [{}, {}]", m_minMotifLen, m_maxMotifLen);
        JOB_LOG_INFO("  Decay mode: {}",
                     m_decayType == DecayType::FastDecay ? "FastDecay" : "");

        // Top 10 most used
        std::vector<std::pair<std::string, uint32_t>> top;
        top.reserve(m_idToStr.size());

        for (const auto& [id, str] : m_idToStr) {
            auto usage_it = m_motifUsage.find(id);
            uint32_t usage = (usage_it != m_motifUsage.end()) ? usage_it->second : 0;
            top.push_back({str, usage});
        }

        const size_t topN = std::min(10UL, top.size());
        if (topN > 0) {
            std::partial_sort(top.begin(), top.begin() + topN, top.end(),
                              [](const auto& a, const auto& b) { return a.second > b.second; });

            JOB_LOG_INFO("  Top {} motifs:", topN);
            for (size_t i = 0; i < topN; ++i) {
                JOB_LOG_INFO("    {}. \"{}\" ({} uses)",
                             i + 1, top[i].first, top[i].second);
            }
        } else {
            JOB_LOG_INFO("  No motifs to display");
        }
    }



private:
    void removeSquatters()
    {
        if (m_idToStr.size() <= kImmutableZone)
            return;

        // bottom 10% by usage
        std::vector<std::pair<uint32_t, uint32_t>> usageList;
        usageList.reserve(m_idToStr.size()); // Avoid reallocs
        for (const auto &[id, str] : m_idToStr) {
            if (id < kImmutableZone)
                continue; // God Particles are immune to photoevaporation
            usageList.push_back({id, m_motifUsage[id]});
        }

        // ascending
        size_t numToEvaporate = usageList.size() / 10;
        std::nth_element(usageList.begin(),  usageList.begin() + numToEvaporate,  usageList.end(),  [](const auto &a, const auto &b) {
            return a.second < b.second;
        });

        // bottom 10%
        for (size_t i = 0; i < numToEvaporate; ++i) {
            uint32_t victimId = usageList[i].first;
            // Time to "transend"
            auto it = m_idToStr.find(victimId);
            if (it != m_idToStr.end()) {
                m_strToId.erase(it->second);
                m_idToStr.erase(it);
                m_motifUsage.erase(victimId);
                m_freeIds.push_back(victimId); // recycle the ID
            }
        }

        for (auto &[id, count] : m_motifUsage) {
            switch(m_decayType){
            case DecayType::FastDecay:
                count >>= 1;
                break;
            case DecayType::SlowDecay:
                count = (count * 9) / 10;
                break;
            case DecayType::Stable:
            default:
                break;
            }
        }
    }

    uint32_t decay() {
        if (m_nextId < kMaxCapacity)
            return m_nextId++;

        // Find least-used motif in mutable zone
        uint32_t victimId = kImmutableZone;
        uint32_t minUsage = UINT32_MAX;

        for (const auto& [id, str] : m_idToStr) {
            if (id < kImmutableZone) continue; // Protected

            uint32_t usage = m_motifUsage[id];
            if (usage < minUsage) {
                minUsage = usage;
                victimId = id;
            }
        }

        // Evict the victim
        auto it = m_idToStr.find(victimId);
        if (it != m_idToStr.end()) {
            m_strToId.erase(it->second);
            m_idToStr.erase(it);
            m_motifUsage.erase(victimId);
        }

        return victimId;
    }

    // map Motif IDs to the High-Z Plane (0.8 to 1.0) ID 0 -> Z=0.8, ID Max -> Z=1.0
    UnicodeLattice idToLattice(uint32_t id, float mass = 1.0f) const
    {
        float x = (float)(id & 0x7F);        // Low 7 bits
        float y = (float)((id >> 7) & 0x7F); // High 7 bits

        return {
            (x / 64.0f) - 1.0f,
            (y / 64.0f) - 1.0f,
            0.9f,
            mass
        };
    }

    uint32_t latticeToId(const UnicodeLattice& p) const
    {
        const int i_x = std::clamp(static_cast<int>((p.x + 1.0f) * 64.0f), 0, 127);
        const int i_y = std::clamp(static_cast<int>((p.y + 1.0f) * 64.0f), 0, 127);
        return static_cast<uint32_t>(i_x | (i_y << 7));
    }

    void addMotif(std::string_view sv)
    {
        if (sv.empty())
            return;

        auto existingIt = m_strToId.find(sv);
        if (existingIt != m_strToId.end())
            return;

        uint32_t id;
        if (!m_freeIds.empty()) {
            id = m_freeIds.back();
            m_freeIds.pop_back();
        } else if (m_nextId < kMaxCapacity) {
            id = m_nextId++;
        } else {
            id = decay();
        }

        // Create string once and move it
        auto [it, inserted] = m_strToId.try_emplace(std::string(sv), id);
        if (inserted) {
            m_idToStr[id] = it->first;
            const size_t len = it->first.size();
            m_maxMotifLen = std::max(m_maxMotifLen, len);
            if (m_minMotifLen == 0)
                m_minMotifLen = len;
            else
                m_minMotifLen = std::min(m_minMotifLen, len);
        }
    }

    struct StringHash {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(std::string_view sv) const
        {
            return std::hash<std::string_view>{}(sv);
        }
        [[nodiscard]] size_t operator()(const std::string& s) const
        {
            return std::hash<std::string>{}(s);
        }
    };

    struct StringEqual {
        using is_transparent = void;
        [[nodiscard]] bool operator()(std::string_view lhs, std::string_view rhs) const {
            return lhs == rhs;
        }
    };

    std::unordered_map<std::string, uint32_t, StringHash, StringEqual>      m_strToId;
    std::unordered_map<uint32_t, std::string>                               m_idToStr;
    std::unordered_map<uint32_t, uint32_t>                                  m_motifUsage;

    size_t                                                                  m_opCount = 0;

    uint32_t                                                                m_nextId = 0;
    size_t                                                                  m_maxMotifLen = 0;
    size_t                                                                  m_minMotifLen = 0 ;// SIZE_MAX;
    std::unique_ptr<CorpusChemist>                                          m_chemist;
    std::vector<uint32_t>                                                   m_freeIds;
    job::core::SplitMix64                                                   m_rng;

    DecayType                                                               m_decayType = DecayType::FastDecay;
};

}
