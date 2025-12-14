#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <cstring>
#include <limits>

#include "ilearn.h"
#include "runner.h"

namespace job::ai::learn {

class BardLearn final : public ILearn {
public:
    static constexpr int kContextLen  = 12;
    static constexpr int kVocabSize   = 95;
    static constexpr int kAsciiOffset = 32;

    explicit BardLearn(threads::ThreadPool::Ptr pool = nullptr) :
        m_pool(std::move(pool))
    {
        m_corpus = "The portal is open. Time flows forward. The dragons are slain.";
        m_inputBuffer.resize(kContextLen);
    }

    [[nodiscard]] float learn(const evo::Genome &genome, uint8_t wsMb = 1) override
    {
        m_lastWsMb = wsMb;

        if (!m_runner)
            m_runner = std::make_unique<infer::Runner>(genome, m_pool, wsMb);
        else
            m_runner->reload(genome);

        float totalNLL = 0.0f; // Negative Log Likelihood
        int samplesProcessed = 0;
        size_t maxSamples = m_corpus.size() - kContextLen;

        for (size_t i = 0; i < maxSamples; ++i) {
            for (int c = 0; c < kContextLen; ++c) {
                char ch = m_corpus[i + c];
                float norm = (float)(ch - kAsciiOffset) / (float)(kVocabSize - 1);
                m_inputBuffer[c] = (norm * 2.0f) - 1.0f;
            }

            cords::ViewR::Extent inputShape{1u, (uint32_t)kContextLen};
            cords::ViewR inputView(m_inputBuffer.data(), inputShape);

            char targetChar = m_corpus[i + kContextLen];
            int targetIdx = std::clamp((int)targetChar - kAsciiOffset, 0, kVocabSize - 1);

            auto output = m_runner->run(inputView, wsMb);
            const float* logits = output.data();

            float maxLogit = -1e30f;
            for (int v = 0; v < kVocabSize; ++v) {
                float val = logits[v];
                uint32_t bits;
                std::memcpy(&bits, &val, 4);
                if ((bits & 0x7F800000u) == 0x7F800000u)
                    return -1e9f; // Die immediately

                if (val > maxLogit)
                    maxLogit = val;
            }

            float sumExp = 0.0f;
            for (int v = 0; v < kVocabSize; ++v)
                sumExp += std::exp(logits[v] - maxLogit);

            float logZ = std::log(sumExp) + maxLogit;
            float nll  = logZ - logits[targetIdx]; // Loss = -log(p_target) ?

            totalNLL += nll;
            samplesProcessed++;
        }

        if (samplesProcessed == 0)
            return 0.0f;

        float avgNLL = totalNLL / (float)samplesProcessed;
        // return std::exp(-avgNLL / 0.5f);
        return std::exp(-avgNLL / std::log((float)kVocabSize));



    }

    [[nodiscard]] std::string hallucinate(int length)
    {
        if (!m_runner)
            return "I have no brain yet...";

        std::string buffer = m_corpus.substr(0, kContextLen);
        std::string result = buffer;
        // std::string result = "";

        for(int i=0; i<length; ++i) {
            for (int c = 0; c < kContextLen; ++c) {
                char ch = buffer[buffer.size() - kContextLen + c];
                float norm = (float)(ch - kAsciiOffset) / (float)(kVocabSize - 1);
                m_inputBuffer[c] = (norm * 2.0f) - 1.0f;
            }

            cords::ViewR::Extent shape{1u, (uint32_t)kContextLen};
            cords::ViewR view(m_inputBuffer.data(), shape);

            auto output = m_runner->run(view, m_lastWsMb);
            const float* logits = output.data();

            int bestIdx = 0;
            float bestVal = -1e30f;
            for(int v=0; v<kVocabSize; ++v) {
                if(logits[v] > bestVal) {
                    bestVal = logits[v];
                    bestIdx = v;
                }
            }

            char nextChar = (char)(bestIdx + kAsciiOffset);
            result += nextChar;
            buffer += nextChar;
        }
        return result;
    }

    [[nodiscard]] uint32_t inputDimension() const noexcept override
    {
        return kContextLen;
    }

    [[nodiscard]] uint32_t outputDimension() const noexcept override
    {
        return kVocabSize;
    }

    [[nodiscard]] static std::unique_ptr<ILearn> create(threads::ThreadPool::Ptr pool)
    {
        return std::make_unique<BardLearn>(std::move(pool));
    }

private:
    threads::ThreadPool::Ptr            m_pool;
    std::unique_ptr<infer::Runner>      m_runner{nullptr};
    std::string                         m_corpus;
    std::vector<float>                  m_inputBuffer;
    uint8_t                             m_lastWsMb = 1;
};

} // namespace job::ai::learn
