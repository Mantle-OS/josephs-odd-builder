#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <limits>
#include <span>

#include <job_logger.h>

#include "ilearn.h"
#include "learn_config.h"
#include "runner.h"

#include "token_factory.h"
#include "itoken.h"
#include "motif_token.h"
#include "byte_lattice.h"

namespace job::ai::learn {

class LanguageLearn final : public ILearn {
public:
    static constexpr int        kParticleDim    = 4;
    static constexpr uint32_t   kByteVocab      = 256;
    static constexpr float      kNegHuge        = -1.0e30f;

    explicit LanguageLearn(const LearnConfig &cfg = LearnPresets::LanguageConfig(),
                        threads::ThreadPool::Ptr pool = nullptr):
        m_pool(std::move(pool)),
        m_cfg(cfg)
    {
        m_tokenizer = token::makeTokenizer(m_cfg.tokenType);
        switch (m_cfg.tokenType) {
        case token::TokenType::Byte:
        case token::TokenType::Char:
            m_outputDim = kByteVocab;
            break;
        case token::TokenType::Ascii:
            m_outputDim = token::AsciiToken::kAsciiVocab;
            break;
        case token::TokenType::Motif:
            m_outputDim = kByteVocab + token::MotifToken::kMaxCapacity;
            break;

        case token::TokenType::BPE:
        case token::TokenType::None:
        default:
            JOB_LOG_WARN("[LanguageLearn] Unsupported token type");
            m_outputDim = kByteVocab;
            break;
        }

        if (m_cfg.corpus && m_cfg.corpus[0] != '\0')
            m_corpus.assign(m_cfg.corpus);
        else
            m_corpus = "The portal is open. Time flows forward. The dragons are slain.";

        // Raw byte view of corpus (already normalized upstream)
        m_corpusBytes.assign(m_corpus.begin(), m_corpus.end());

        // Wire corpus into motif tokenizer if needed
        if (auto *motif = dynamic_cast<token::MotifToken*>(m_tokenizer.get()))
            motif->setCorpus(m_corpus, m_pool);

        // Buffers
        m_inputBuffer.resize(m_cfg.contextLen * kParticleDim);
        m_window.resize(m_cfg.contextLen);
        m_tmpDecode.resize(4096);
    }

    // Token evolution hook
    void onTokenTime([[maybe_unused]] uint64_t generation,
                        uint64_t seed) override
    {
        if (m_tokenizer)
            m_tokenizer->mutate(seed);
    }

    // Training step
    [[nodiscard]] float learn(const evo::Genome &genome) override
    {
        const uint32_t wsSize =
            (m_cfg.initWsMb > 0) ? m_cfg.initWsMb : 1;

        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, wsSize);
        else
            m_runner->reload(genome);

        // Encode corpus -> atoms
        m_atoms.resize(m_corpusBytes.size());
        const std::size_t atomCount =
            m_tokenizer->encode(
                std::span<const uint8_t>(m_corpusBytes),
                std::span<token::ByteLattice>(m_atoms),
                1.0f);

        m_atoms.resize(atomCount);

        if (m_atoms.size() <= m_cfg.contextLen)
            return 0.0f;

        float totalNLL = 0.0f;
        std::size_t samplesProcessed = 0;

        const std::size_t maxSamples =
            m_atoms.size() - m_cfg.contextLen;

        const uint32_t flatInputSize =
            m_cfg.contextLen * kParticleDim;

        auto inputShape = cords::makeBS(1u, flatInputSize);

        for (std::size_t i = 0; i < maxSamples; ++i) {

            // Fill context window
            for (uint32_t c = 0; c < m_cfg.contextLen; ++c) {
                const auto &p = m_atoms[i + c];
                const std::size_t off = c * kParticleDim;
                m_inputBuffer[off + 0] = p.x;
                m_inputBuffer[off + 1] = p.y;
                m_inputBuffer[off + 2] = p.z;
                m_inputBuffer[off + 3] = p.mass;
            }

            cords::ViewR inputView(m_inputBuffer.data(), inputShape);
            auto output = m_runner->run(inputView, wsSize);
            const float *logits = output.data();

            const token::ByteLattice &targetP = m_atoms[i + m_cfg.contextLen];

            uint32_t targetIdx = latticeToIndex(targetP);

            if (targetIdx >= m_outputDim)
                targetIdx = 0;

            // Softmax + NLL
            float maxLogit = kNegHuge; // -std::numeric_limits<float>::infinity();
            for (uint32_t v = 0; v < m_outputDim; ++v)
                maxLogit = std::max(maxLogit, logits[v]);

            float sumExp = 0.0f;
            for (uint32_t v = 0; v < m_outputDim; ++v)
                sumExp += std::exp(logits[v] - maxLogit);

            if (sumExp <= 0.0f)
                return -1e9f;

            const float logZ = std::log(sumExp) + maxLogit;
            const float nll  = logZ - logits[targetIdx];

            if (punishLearner(nll))
                return -1e9f;

            totalNLL += nll;
            ++samplesProcessed;
        }

        if (samplesProcessed == 0)
            return 0.0f;

        const float avgNLL = totalNLL / samplesProcessed;
        const float baseline = std::log((float)m_outputDim);
        const float norm = avgNLL / baseline;
        m_fitness = m_cfg.targetFitness / (1.0f + norm);
        return m_fitness;
    }

    // API for applciations to call hallucinate.
    [[nodiscard]] std::string say(std::size_t targetBytes, const evo::Genome &genome)
    {
        auto fit = learn(genome);
        if(m_lastFitness < fit){
            m_lastFitness = fit;
            if(m_lastFitness >= m_cfg.targetFitness)
                m_done.store(true, std::memory_order_relaxed);
        }
        return hallucinate(targetBytes);
    }

    [[nodiscard]] uint32_t inputDimension() const noexcept override
    {
    return m_cfg.contextLen * kParticleDim;
    }

    [[nodiscard]] uint32_t outputDimension() const noexcept override
    {
    return m_outputDim;
    }

    [[nodiscard]] static std::unique_ptr<ILearn> create(const LearnConfig &cfg, threads::ThreadPool::Ptr pool)
    {
    return std::make_unique<LanguageLearn>(cfg, std::move(pool));
    }
    token::IToken *tokenizer() {return m_tokenizer.get();}

private:
    // Lattice <-> index mapping
    static uint32_t motifLatticeToId(const token::ByteLattice &p) noexcept
    {
        const int ix = std::clamp((int)std::lrintf((p.x + 1.0f) * 64.0f), 0, 127);
        const int iy = std::clamp((int)std::lrintf((p.y + 1.0f) * 64.0f), 0, 127);
        return (uint32_t)(ix | (iy << 7));
    }

    uint32_t latticeToIndex(const token::ByteLattice &p) const noexcept
    {
        const uint8_t b = token::ByteLattice::decode(p);

        switch (m_cfg.tokenType) {
            case token::TokenType::Ascii: {
                uint8_t bb = b;
                if (bb < token::AsciiToken::kAsciiMin)
                    bb = (uint8_t)token::AsciiToken::kAsciiMin;

                if (bb > token::AsciiToken::kAsciiMax)
                    bb = (uint8_t)token::AsciiToken::kAsciiMax;

                return uint32_t(bb - token::AsciiToken::kAsciiMin); // 0..94
            }
            case token::TokenType::Motif:
                // return kByteVocab + motifLatticeToId(p);
                if (token::MotifToken::isMotifLane(p))
                    return kByteVocab + token::MotifToken::latticeToId(p);
                return uint32_t(b);

            default:
                return uint32_t(b);
            }
    }

    token::ByteLattice indexToLattice(uint32_t idx) const noexcept
    {
        switch (m_cfg.tokenType) {
        case token::TokenType::Ascii: {

            if (idx >= token::AsciiToken::kAsciiVocab)
                idx = token::AsciiToken::kAsciiVocab - 1;

            return token::ByteLattice::encode(uint8_t(token::AsciiToken::kAsciiMin + idx), 1.0f);
        }

        case token::TokenType::Motif:
            if (idx < kByteVocab)
                return token::ByteLattice::encode(uint8_t(idx), 1.0f);
            return token::MotifToken::idToLattice(idx - kByteVocab, 1.0f);
        default:
            if (idx < kByteVocab)
                return token::ByteLattice::encode(uint8_t(idx), 1.0f);
        }

        // motif branch
        const uint32_t id = idx - kByteVocab;
        const float x = float(id & 0x7F);
        const float y = float((id >> 7) & 0x7F);
        return { (x * 0.015625f) - 1.0f, (y * 0.015625f) - 1.0f, 1.0f, 1.0f };
    }

    [[nodiscard]] std::string hallucinate(std::size_t targetBytes)
    {
        if (!m_runner)
            return "I have no brain yet...";

        if (m_atoms.empty()) {
            m_atoms.resize(m_corpusBytes.size());
            const std::size_t atomCount = m_tokenizer->encode( m_corpusBytes, m_atoms, 1.0f);
            m_atoms.resize(atomCount);
        }

        if (m_atoms.size() < m_cfg.contextLen)
            return "Not enough data...";

        // Seed context from the start of the known universe
        std::copy_n(m_atoms.begin(), m_cfg.contextLen, m_window.begin());

        std::string result;
        result.reserve(targetBytes); // Optimization: Prevent re-allocs

        // PREPARE INPUT SHAPES ONCE
        const uint32_t flatInputSize = m_cfg.contextLen * kParticleDim;
        auto inputShape = cords::makeBS(1u, flatInputSize);

        // LIMITER: Stop when we hit the byte target (or a safety limit of steps)
        int safetySteps = 0;
        const int maxSafetySteps = targetBytes * 2;

        while (result.size() < targetBytes && safetySteps++ < maxSafetySteps) {

            // Fill buffer from window
            for (uint32_t c = 0; c < m_cfg.contextLen; ++c) {
                const auto &p = m_window[c];
                const std::size_t off = c * kParticleDim;
                m_inputBuffer[off + 0] = p.x;
                m_inputBuffer[off + 1] = p.y;
                m_inputBuffer[off + 2] = p.z;
                m_inputBuffer[off + 3] = p.mass;
            }

            cords::ViewR inputView(m_inputBuffer.data(), inputShape);
            auto output = m_runner->run(inputView, m_cfg.initWsMb); // Reuse WS size
            const float *logits = output.data();

            // Greedy Search
            uint32_t bestIdx = 0;
            float bestVal = -1e9f;
            for (uint32_t v = 0; v < m_outputDim; ++v) {
                if (logits[v] > bestVal) { bestVal = logits[v]; bestIdx = v; }
            }

            token::ByteLattice nextP = indexToLattice(bestIdx);

            // Slide window
            std::move(m_window.begin() + 1, m_window.end(), m_window.begin());
            m_window.back() = nextP;

            // Decode
            const std::size_t written = m_tokenizer->decode(
                std::span<const token::ByteLattice>(&nextP, 1),
                std::span<uint8_t>(m_tmpDecode));

            result.append(reinterpret_cast<const char*>(m_tmpDecode.data()), written);
        }

        // Exact trim if we overshot due to a long motif
        if (result.size() > targetBytes)
            result.resize(targetBytes);

        //local fit

        return result;
    }


    threads::ThreadPool::Ptr            m_pool;
    std::unique_ptr<infer::Runner>      m_runner;

    LearnConfig                         m_cfg;

    std::string                         m_corpus;
    std::vector<uint8_t>                m_corpusBytes;

    std::unique_ptr<token::IToken>      m_tokenizer;

    std::vector<token::ByteLattice>     m_atoms;
    std::vector<token::ByteLattice>     m_window;

    std::vector<float>                  m_inputBuffer;
    std::vector<uint8_t>                m_tmpDecode;

    uint32_t                            m_outputDim = 0;
    float                               m_lastFitness = 0.0f;
    std::atomic<bool>                   m_done{false};

};

} // namespace job::ai::learn
