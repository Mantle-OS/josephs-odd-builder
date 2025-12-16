#pragma once
#include <string_view>
#include <unordered_map>
#include <string>
#include <vector>
#include <random>

namespace job::ai::token {

class CorpusChemist {
public:
    explicit CorpusChemist(std::string_view corpus) :
    m_corpus(corpus)
    {

    }
    std::string findReactiveMolecule(int len, int samples, uint64_t seed)
    {
        if (m_corpus.size() < (size_t)len)
            return "";

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, m_corpus.size() - len);

        std::unordered_map<std::string_view, int> collisions;
        std::string_view bestCandidate;
        int maxReactions = 0;

        // Monte Carlo Sampling
        for(int i=0; i<samples; ++i) {
            size_t pos = dist(rng);
            std::string_view fragment = m_corpus.substr(pos, len);
            int reactions = ++collisions[fragment];
            if (reactions > maxReactions) {
                maxReactions = reactions;
                bestCandidate = fragment;
            }
        }

        if (maxReactions > 1)
            return std::string(bestCandidate);
        return "";
    }

private:
    std::string_view m_corpus;
};

}
