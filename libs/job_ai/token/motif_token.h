#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <atomic>
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
    // static constexpr uint32_t kMaxCapacity   = 16384; // 128x128 plane
    static constexpr uint32_t kMaxCapacity   = 4096;
    static constexpr uint32_t kImmutableZone = 100;

    MotifToken()
    {
        m_motifUsage = std::make_unique<std::atomic<uint32_t>[]>(kMaxCapacity);
        for(size_t i=0; i<kMaxCapacity; ++i)
            m_motifUsage[i] = 0;

        // "God Particles" (immutable-ish starter motifs)
        addMotif("The ");
        addMotif(" the ");
        addMotif(" of ");
        addMotif(" and ");
        addMotif(" to ");
        addMotif(" in ");
        addMotif("ing");
        addMotif("tion");
        addMotif("Job");    // Self-awareness :>)
        addMotif("Joseph");
        addMotif("Odd");
        addMotif("Builder");
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

                        // atomic increment
                        m_motifUsage[id].fetch_add(1, std::memory_order_relaxed);

                        // emit motif atom
                        output[outCount++] = idToLattice(id, mass);
                        i += scanLen;
                        foundMotif = true;
                        break;
                    }

                    // prevent unsigned wrap
                    if (scanLen == m_minMotifLen)
                        break;
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
        if (!m_chemist) return;

        lookForSquatters();

        core::SplitMix64 rng(seed);

        // ... (Your bondLen logic from the previous step) ...
        // Keep the longer lengths (5, 6, 7) to catch "Physics"
        int bondLen = 2;
        const float r = rng.nextFloat();
        if (r < 0.30f)
            bondLen = 3;
        else if (r < 0.60f)
            bondLen = 4;
        else if (r < 0.85f)
            bondLen = 5;
        else if (r < 0.95f)
            bondLen = 6;
        else
            bondLen = 7;

        // Get candidate
        std::string newMolecule = m_chemist->findReactiveMolecule(bondLen, 2000, seed);

        bool isToxic = false;

        // 1. Ban tokens that cross a newline (Phase Transition) allow "\n" itself, but not ".\n<"
        if (newMolecule.size() > 1 && newMolecule.find('\n') != std::string::npos)
            isToxic = true;


        // Ban tokens that contain a Tag Opener '<' in the middle
        //    We allow "<User", but not "r><U" or ".\n<"
        //    Find '<'. If it exists and is NOT at index 0, it's a bridge.
        size_t tagPos = newMolecule.find('<');
        if (tagPos != std::string::npos && tagPos > 0)
            isToxic = true;


        // Ban tokens that contain a Tag Closer '>' in the middle
        // We allow ">" or "> " but not "er>W" (User>Who)
        size_t closePos = newMolecule.find('>');
        if (closePos != std::string::npos && closePos < newMolecule.size() - 1)
            isToxic = true;


        if (!isToxic && !newMolecule.empty() && ansi::utils::Utf8Decoder::isValid(newMolecule))
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
        JOB_LOG_INFO("  Vocabulary: {}/{}", m_strToId.size(), kMaxCapacity);
        JOB_LOG_INFO("  Free IDs: {}", m_freeIds.size());
        JOB_LOG_INFO("  Length range: [{}, {}]", m_minMotifLen, m_maxMotifLen);
        JOB_LOG_INFO("  Decay mode: {}",
                        m_decayType == DecayType::FastDecay ? "FastDecay" :
                        m_decayType == DecayType::SlowDecay ? "SlowDecay" : "Stable");

        std::vector<std::pair<std::string, uint32_t>> top;
        top.reserve(m_idToStr.size());

        for (const auto &[id, str] : m_idToStr) {
            uint32_t usage = 0;
            if (id < kMaxCapacity)
                usage = m_motifUsage[id].load(std::memory_order_relaxed);
            top.push_back({str, usage});
        }

        const std::size_t topN = std::min<std::size_t>(10, top.size());
        if (topN == 0) {
            JOB_LOG_INFO("  No motifs to display");
            return;
        }

        std::partial_sort(top.begin(), top.begin() + topN, top.end(),
                [](const auto &a, const auto &b) { return a.second > b.second; });

        JOB_LOG_INFO("  Top {} motifs:", topN);
        for (std::size_t i = 0; i < topN; ++i) {
            JOB_LOG_INFO("    {}. \"{}\" ({} uses)", i + 1, top[i].first, top[i].second);
        }
     }

    void lookForSquatters()
    {
        if (m_opCount++ % 1000 == 0)
            removeSquatters();
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

            const uint32_t usage = m_motifUsage[id].load(std::memory_order_relaxed);

            if (usage < minUsage) {
                minUsage = usage;
                victimId = id;
            }
        }

        auto it = m_idToStr.find(victimId);
        if (it != m_idToStr.end()) {
            m_strToId.erase(it->second);
            m_idToStr.erase(it);

            // don't erase() the array index; just reset the counter to 0.
            m_motifUsage[victimId].store(0, std::memory_order_relaxed);
        }

        return victimId;
    }

private:
    [[nodiscard]] static constexpr bool isMotifLane(const ByteLattice &p) noexcept
    {
        return p.z > 0.5f;
   }

    ByteLattice idToLattice(uint32_t id, float mass = 1.0f) const
    {
        // const float x = (float)(id & 0x7F);         // low 7
        // const float y = (float)((id >> 7) & 0x7F);  // high 7

        // return {
        //     (x * 0.015625f) - 1.0f,
        //     (y * 0.015625f) - 1.0f,
        //     1.0f, // motif lane
        //     mass
        // };

        const float x = (float)(id & 0x3F);        // Low 6 bits (0-63)
        const float y = (float)((id >> 6) & 0x3F); // High 6 bits (0-63)

        // Map [0..63] to [-1.0..1.0]
        // Step size = 2.0 / 64.0 = 0.03125 (1/32)
        return {
            (x * 0.03125f) - 1.0f,
            (y * 0.03125f) - 1.0f,
            1.0f, // motif lane
            mass
        };
    }

    uint32_t latticeToId(const ByteLattice &p) const
    {
        // Snap to grid like ByteLattice decode does.
        // const int i_x = std::clamp((int)std::lrintf((p.x + 1.0f) * 64.0f), 0, 127);
        // const int i_y = std::clamp((int)std::lrintf((p.y + 1.0f) * 64.0f), 0, 127);
        // return (uint32_t)(i_x | (i_y << 7));

        const int i_x = std::clamp((int)std::lrintf((p.x + 1.0f) * 32.0f), 0, 63);
        const int i_y = std::clamp((int)std::lrintf((p.y + 1.0f) * 32.0f), 0, 63);
        // Reassemble ID: Y << 6 | X
        return (uint32_t)(i_x | (i_y << 6));

    }



    void removeSquatters() {
        if (m_idToStr.size() <= kImmutableZone)
            return;

        uint32_t victimId = 0;
        uint32_t minUsage = UINT32_MAX;

        for(const auto& [id, str] : m_idToStr) {
            if (id < kImmutableZone)
                continue;
            uint32_t u = m_motifUsage[id].load(std::memory_order_relaxed);
            if (u < minUsage) {
                minUsage = u;
                victimId = id;
            }
        }

        if (victimId != 0) {
            auto it = m_idToStr.find(victimId);
            if (it != m_idToStr.end()) {
                m_strToId.erase(it->second);
                m_idToStr.erase(it);
                m_motifUsage[victimId].store(0, std::memory_order_relaxed);
                m_freeIds.push_back(victimId);
            }
        }

        // decay
        for(size_t i=0; i<kMaxCapacity; ++i) {
            uint32_t old = m_motifUsage[i].load(std::memory_order_relaxed);
            if (old > 0)
                m_motifUsage[i].store(old >> 1, std::memory_order_relaxed);
        }
    }


    void addMotif(std::string_view sv) {
        if (sv.empty() || m_strToId.count(sv))
            return;

        uint32_t id;
        if (!m_freeIds.empty()) {
            id = m_freeIds.back();
            m_freeIds.pop_back();
        } else if (m_nextId < kMaxCapacity) {
            id = m_nextId++;
        } else {
            return;
        }

        auto [it, inserted] = m_idToStr.emplace(id, std::string(sv));
        std::string_view permanentView = it->second;
        m_strToId.emplace(permanentView, id);
        m_motifUsage[id].store(0, std::memory_order_relaxed);

        m_maxMotifLen = std::max(m_maxMotifLen, sv.size());
        m_minMotifLen = (m_minMotifLen == 0) ?
                            sv.size() :
                            std::min(m_minMotifLen, sv.size());
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

    bool save(std::ostream &os) const override
    {
        const uint32_t magic = 0xFEEDBEEF;
        const uint32_t ver   = 1;
        os.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        os.write(reinterpret_cast<const char*>(&ver), sizeof(ver));

        os.write(reinterpret_cast<const char*>(&m_nextId), sizeof(m_nextId));
        os.write(reinterpret_cast<const char*>(&m_maxMotifLen), sizeof(m_maxMotifLen));
        os.write(reinterpret_cast<const char*>(&m_minMotifLen), sizeof(m_minMotifLen));

        // Save Dictionary
        const uint32_t count = static_cast<uint32_t>(m_idToStr.size());
        os.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto &[id, str] : m_idToStr) {
            os.write(reinterpret_cast<const char*>(&id), sizeof(id));

            const uint32_t len = static_cast<uint32_t>(str.size());
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(str.data(), len);

            uint32_t usage = 0;
            if (id < kMaxCapacity)
                usage = m_motifUsage[id].load(std::memory_order_relaxed);
            os.write(reinterpret_cast<const char*>(&usage), sizeof(usage));
        }
        return os.good();
    }

    bool load(std::istream &is) override
    {
        uint32_t magic = 0, ver = 0;
        is.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0xFEEDBEEF) return false;
        is.read(reinterpret_cast<char*>(&ver), sizeof(ver));
        if (ver != 1) return false;

        // Reset everything before loading
        m_strToId.clear();
        m_idToStr.clear();
        m_freeIds.clear();
        for(size_t i=0; i<kMaxCapacity; ++i) m_motifUsage[i].store(0);

        is.read(reinterpret_cast<char*>(&m_nextId), sizeof(m_nextId));
        is.read(reinterpret_cast<char*>(&m_maxMotifLen), sizeof(m_maxMotifLen));
        is.read(reinterpret_cast<char*>(&m_minMotifLen), sizeof(m_minMotifLen));

        uint32_t count = 0;
        is.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t id = 0;
            is.read(reinterpret_cast<char*>(&id), sizeof(id));

            uint32_t len = 0;
            is.read(reinterpret_cast<char*>(&len), sizeof(len));

            std::string str;
            str.resize(len);
            // Robust read into string buffer
            is.read(&str[0], len);

            uint32_t usage = 0;
            is.read(reinterpret_cast<char*>(&usage), sizeof(usage));

            // Restore Maps
            m_idToStr.emplace(id, str);
            m_strToId.emplace(str, id);

            if (id < kMaxCapacity)
                m_motifUsage[id].store(usage, std::memory_order_relaxed);
        }
        return is.good();
    }

private:
    std::unordered_map<std::string, uint32_t, StringHash, StringEqual>  m_strToId;
    std::unique_ptr<std::atomic<uint32_t>[]>                            m_motifUsage;
    std::unordered_map<uint32_t, std::string>                           m_idToStr;

    std::size_t                                                         m_opCount = 0;

    uint32_t                                                            m_nextId = 0;
    std::size_t                                                         m_maxMotifLen = 0;
    std::size_t                                                         m_minMotifLen = 0;

    std::unique_ptr<CorpusChemist>                                      m_chemist;
    std::vector<uint32_t>                                               m_freeIds;

    DecayType                                                           m_decayType = DecayType::FastDecay;
};

} // namespace job::ai::token
