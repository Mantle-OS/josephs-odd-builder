#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <cstring>
#include <limits>

#include "ilearn.h"
#include "learn_config.h"
#include "runner.h"
#include "token_factory.h"
#include "itoken.h"
#include "motif_token.h"

namespace job::ai::learn {

class BardLearn final : public ILearn {
public:
    static constexpr int kParticleDim = 4;
    static constexpr uint32_t kCharOffset = 0x100000;
    static constexpr int kAsciiVocab   = 95; // 32..126
    static constexpr int kAsciiOffset  = 32;

    explicit BardLearn(const LearnConfig &cfg = LearnPresets::BardConfig(),
                       threads::ThreadPool::Ptr pool = nullptr) :
        m_pool(std::move(pool)),
        m_cfg(cfg)
    {
        m_tokenizer = token::makeTokenizer(m_cfg.tokenType);


        if (m_cfg.tokenType == token::TokenType::Char || m_cfg.tokenType == token::TokenType::Ascii)
            m_outputDim = 96; // ASCII printable range
        else
            m_outputDim = 16384; // Motif Lattice


        // Pointer safety
        if (m_cfg.corpus && m_cfg.corpus[0] != '\0')
            m_corpus = m_cfg.corpus;
         else
            m_corpus = "The portal is open. Time flows forward. The dragons are slain.";


        if (auto motif = dynamic_cast<token::MotifToken*>(m_tokenizer.get()))
            motif->setCorpus(m_corpus);

        m_inputBuffer.resize(m_cfg.contextLen * kParticleDim);
    }

    void onTokenTime([[maybe_unused]]uint64_t generation, uint64_t seed) override
    {
        if (m_tokenizer) m_tokenizer->mutate(seed);
    }

    [[nodiscard]] float learn(const evo::Genome &genome) override
    {
        uint8_t wsSize = (m_cfg.initWsMb > 0) ? m_cfg.initWsMb : 1;

        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, wsSize);
        else
            m_runner->reload(genome);

        auto particles = m_tokenizer->encode(m_corpus);
        if (particles.size() <= m_cfg.contextLen) return 0.0f;

        float totalNLL = 0.0f;
        int samplesProcessed = 0;
        size_t maxSamples = particles.size() - m_cfg.contextLen;

        const uint32_t flatInputSize = m_cfg.contextLen * kParticleDim;
        auto inputShape = cords::makeBS(1u, flatInputSize);

        for (size_t i = 0; i < maxSamples; ++i) {
            // Fill Context
            for (uint32_t c = 0; c < m_cfg.contextLen; ++c) {
                const auto& p = particles[i + c];
                size_t offset = c * kParticleDim;
                m_inputBuffer[offset + 0] = p.x;
                m_inputBuffer[offset + 1] = p.y;
                m_inputBuffer[offset + 2] = p.z;
                m_inputBuffer[offset + 3] = p.mass;
            }

            cords::ViewR inputView(m_inputBuffer.data(), inputShape);
            auto output = m_runner->run(inputView, wsSize);
            const float *logits = output.data();

            uint32_t targetIdx = 0;
            const auto& targetP = particles[i + m_cfg.contextLen];

            if (m_cfg.tokenType == token::TokenType::Motif) {
                targetIdx = latticeToId(targetP);
            }  else if (m_cfg.tokenType == token::TokenType::Char) {
                targetIdx = (uint32_t)token::UnicodeLattice::decode(targetP);
            } else if (m_cfg.tokenType == token::TokenType::Ascii) {
                float t = (targetP.x + 1.0f) * 0.5f;
                int idx = (int)std::round(t * 94.0f);
                if (idx < 0)
                    idx = 0;
                targetIdx = (uint32_t)idx;
            }

            if (targetIdx >= m_outputDim)
                targetIdx = 0;

            // Softmax & NLL
            float maxLogit = -1e30f;
            for (uint32_t v = 0; v < m_outputDim; ++v)
                if (logits[v] > maxLogit)
                    maxLogit = logits[v];


            float sumExp = 0.0f;
            for (uint32_t v = 0; v < m_outputDim; ++v)
                sumExp += std::exp(logits[v] - maxLogit);

            if (sumExp <= 0.0f)
                return -1e9f;

            float logZ = std::log(sumExp) + maxLogit;
            float nll  = logZ - logits[targetIdx];

            if (punishLearner(nll))
                return -1e9f;

            totalNLL += nll;
            samplesProcessed++;
        }

        if (samplesProcessed == 0)
            return 0.0f;

        float avgNLL = totalNLL / (float)samplesProcessed;
        float baseline = std::log((float)m_outputDim);
        float norm = avgNLL / baseline;

        return m_cfg.targetFitness / (1.0f + norm);
    }

    [[nodiscard]] std::string hallucinate(int length)
    {
        if (!m_runner)
            return "I have no brain yet...";

        auto contextParticles = m_tokenizer->encode(m_corpus.substr(0, std::min<size_t>(100, m_corpus.size())));
        if (contextParticles.size() < m_cfg.contextLen)
            return "Not enough data...";

        std::vector<token::UnicodeLattice> window;
        window.insert(window.end(), contextParticles.begin(), contextParticles.begin() + m_cfg.contextLen);

        std::string result;
        const uint32_t flatInputSize = m_cfg.contextLen * kParticleDim;
        auto inputShape = cords::makeBS(1u, flatInputSize);

        for(int i=0; i<length; ++i) {
            for (uint32_t c = 0; c < m_cfg.contextLen; ++c) {
                const auto &p = window[c];
                size_t offset = c * kParticleDim;
                m_inputBuffer[offset + 0] = p.x;
                m_inputBuffer[offset + 1] = p.y;
                m_inputBuffer[offset + 2] = p.z;
                m_inputBuffer[offset + 3] = p.mass;
            }

            cords::ViewR inputView(m_inputBuffer.data(), inputShape);
            auto output = m_runner->run(inputView, m_cfg.initWsMb);
            const float *logits = output.data();

            int bestIdx = 0;
            float bestVal = -1e30f;
            for(uint32_t v=0; v<m_outputDim; ++v) {
                if(logits[v] > bestVal) {
                    bestVal = logits[v];
                    bestIdx = (int)v;
                }
            }

            token::UnicodeLattice nextP;
            if (m_cfg.tokenType == token::TokenType::Char) {
                char c = (char)bestIdx;
                if (c < 32 && c != '\n') c = '.';
                result += c;
                nextP = token::UnicodeLattice::encode((char32_t)bestIdx);
            }  else if (m_cfg.tokenType == token::TokenType::Ascii) {
                char c = (char)(bestIdx + kAsciiOffset);
                if (c > 126)
                    c = '~';
                result += c;
                nextP.x = ((float)bestIdx / (float)(kAsciiVocab - 1)) * 2.0f - 1.0f;
                nextP.y = 0.0f; nextP.z = 0.0f; nextP.mass = 1.0f;
            } else if (m_cfg.tokenType == token::TokenType::Motif) {
                nextP = idToLattice(bestIdx);
                std::vector<token::UnicodeLattice> decodeVec = { nextP };
                result += m_tokenizer->decode(decodeVec);
            }  else {
                nextP.x = (float)bestIdx;
                std::vector<token::UnicodeLattice> decodeVec = { nextP };
                result += m_tokenizer->decode(decodeVec);
            }

            window.erase(window.begin());
            window.push_back(nextP);
        }
        return result;
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
        return std::make_unique<BardLearn>(cfg, std::move(pool));
    }

private:

    uint32_t latticeToId(const token::UnicodeLattice& p) const
    {
        int i_x = std::clamp((int)((p.x + 1.0f) * 64.0f), 0, 127);
        int i_y = std::clamp((int)((p.y + 1.0f) * 64.0f), 0, 127);
        return (uint32_t)(i_x | (i_y << 7));
    }

    token::UnicodeLattice idToLattice(uint32_t id) const
    {
        float x = (float)(id & 0x7F);
        float y = (float)((id >> 7) & 0x7F);
        return { (x / 64.0f) - 1.0f, (y / 64.0f) - 1.0f, 0.9f, 1.0f };
    }

    threads::ThreadPool::Ptr            m_pool;
    std::unique_ptr<infer::Runner>      m_runner{nullptr};
    std::string                         m_corpus;
    std::vector<float>                  m_inputBuffer;
    LearnConfig                         m_cfg;
    std::unique_ptr<token::IToken>      m_tokenizer;
    uint32_t                            m_outputDim;
};

} // namespace job::ai::learn
