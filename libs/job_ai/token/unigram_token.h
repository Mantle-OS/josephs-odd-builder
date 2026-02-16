#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <memory>

#include "itoken.h"
#include "byte_lattice.h"

namespace job::ai::token {

// A lightweight Trie node for O(L) prefix lookup
struct TrieNode {
    std::unordered_map<uint8_t, std::unique_ptr<TrieNode>> children;
    uint32_t tokenId = 0xFFFFFFFF; // Invalid ID
    float score = 0.0f;            // Log Probability
};

class UnigramToken final : public IToken {
public:
    static constexpr uint32_t kBaseVocab = 256;
    static constexpr float kUnkScore = -10.0f; // Penalty for unknown chars

    UnigramToken() {
        m_root = std::make_unique<TrieNode>();
        // Initialize base vocab (bytes) with low scores so we always have a fallback
        for(uint32_t i=0; i<kBaseVocab; ++i) {
            std::string s(1, (char)i);
            addTokenToTrie(s, i, -50.0f); // Low score for raw bytes
            m_vocab[i] = s;
        }
    }

    TokenType type() const  { return TokenType::Unigram; } // Assuming you add this enum

    // --------------------------------------------------------
    // ENCODE: Viterbi Algorithm (Dynamic Programming)
    // --------------------------------------------------------
    std::size_t encode(std::span<const uint8_t> input, std::span<ByteLattice> output, float noise) override
    {
        if (input.empty() || output.empty()) return 0;
        const size_t n = input.size();

        // DP State: best_score[i] is the max log-prob to generate input[0...i]
        std::vector<float> best_score(n + 1, -std::numeric_limits<float>::infinity());
        std::vector<uint32_t> best_token_len(n + 1, 0); // Length of the last token ending at i
        std::vector<uint32_t> best_token_id(n + 1, 0);  // ID of the last token ending at i

        best_score[0] = 0.0f;

        // Forward Pass (Find best path)
        for (size_t i = 0; i < n; ++i) {
            if (best_score[i] == -std::numeric_limits<float>::infinity()) continue; // Unreachable

            // Traverse Trie to find all tokens starting at input[i]
            TrieNode* node = m_root.get();
            for (size_t len = 1; i + len <= n; ++len) {
                uint8_t c = input[i + len - 1];
                if (node->children.find(c) == node->children.end()) break;
                node = node->children[c].get();

                if (node->tokenId != 0xFFFFFFFF) {
                    // Valid token found! Check if this path is better.
                    float new_score = best_score[i] + node->score;
                    if (new_score > best_score[i + len]) {
                        best_score[i + len] = new_score;
                        best_token_len[i + len] = (uint32_t)len;
                        best_token_id[i + len] = node->tokenId;
                    }
                }
            }
        }

        // Backward Pass (Reconstruct path)
        if (best_score[n] == -std::numeric_limits<float>::infinity()) // FAST MATH YOU CAN NOT USE THIS CALL
        {
            // job::core::isSafeFinite()
            // Fallback: If Viterbi fails (shouldn't happen with base vocab), just emit raw bytes
            // Simplified logic: return 0 or handle error
            return 0;
        }

        std::vector<uint32_t> path;
        size_t idx = n;
        while (idx > 0) {
            path.push_back(best_token_id[idx]);
            idx -= best_token_len[idx];
        }
        std::reverse(path.begin(), path.end());

        // Output
        size_t writeCount = std::min(path.size(), output.size());
        for (size_t i = 0; i < writeCount; ++i) {
            output[i] = idToLattice(path[i]);
        }

        return writeCount;
    }

    // --------------------------------------------------------
    // DECODE: IDs -> Text
    // --------------------------------------------------------
    std::size_t decode(std::span<const ByteLattice> input, std::span<uint8_t> output) override
    {
        std::vector<uint8_t> buffer;
        buffer.reserve(input.size() * 4);

        for (const auto& lattice : input) {
            uint32_t id = latticeToId(lattice);
            if (m_vocab.find(id) != m_vocab.end()) {
                const std::string& s = m_vocab[id];
                buffer.insert(buffer.end(), s.begin(), s.end());
            }
        }

        size_t writeCount = std::min(buffer.size(), output.size());
        std::copy_n(buffer.begin(), writeCount, output.begin());
        return writeCount;
    }

    // Unigram is typically static after training
    void mutate(uint64_t /*seed*/) override { }

    // --------------------------------------------------------
    // SERIALIZATION
    // --------------------------------------------------------
    bool save(std::ostream &os) const override {
        IToken::save(os); // Writes Type ID

        uint32_t count = (uint32_t)m_vocab.size();
        os.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& [id, str] : m_vocab) {
            // Write ID
            os.write(reinterpret_cast<const char*>(&id), sizeof(id));

            // Write Score
            float score = m_scores.at(id);
            os.write(reinterpret_cast<const char*>(&score), sizeof(score));

            // Write String
            uint32_t len = (uint32_t)str.size();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(str.data(), len);
        }
    }

    bool load(std::istream &is) override {
        // Reset Trie and maps
        m_root = std::make_unique<TrieNode>();
        m_vocab.clear();
        m_scores.clear();

        uint32_t count;
        is.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t id;
            float score;
            uint32_t len;

            is.read(reinterpret_cast<char*>(&id), sizeof(id));
            is.read(reinterpret_cast<char*>(&score), sizeof(score));
            is.read(reinterpret_cast<char*>(&len), sizeof(len));

            std::string s(len, '\0');
            is.read(s.data(), len);

            // Rebuild state
            m_vocab[id] = s;
            m_scores[id] = score;
            addTokenToTrie(s, id, score);
        }
    }

    // --------------------------------------------------------
    // MANUAL TRAINING (Simplified)
    // --------------------------------------------------------
    // In reality, you'd load a SentencePiece model file here.
    // This allows you to manually inject a dictionary.
    void trainFromDictionary(const std::vector<std::pair<std::string, float>>& dict) {
        uint32_t nextId = kBaseVocab;

        for (const auto& [word, score] : dict) {
            // Don't overwrite base bytes
            if (word.size() == 1) continue;

            m_vocab[nextId] = word;
            m_scores[nextId] = score;
            addTokenToTrie(word, nextId, score);
            nextId++;
        }
    }

private:
    void addTokenToTrie(const std::string& s, uint32_t id, float score) {
        TrieNode* node = m_root.get();
        for (char c : s) {
            if (node->children.find((uint8_t)c) == node->children.end()) {
                node->children[(uint8_t)c] = std::make_unique<TrieNode>();
            }
            node = node->children[(uint8_t)c].get();
        }
        node->tokenId = id;
        node->score = score;
    }

    // Standard lattice mapping (same as BPE/Motif)
    ByteLattice idToLattice(uint32_t id) const {
        float x = (float)(id & 0xFFF) / 2048.0f - 1.0f;
        float y = (float)((id >> 12) & 0xFFF) / 2048.0f - 1.0f;
        return { x, y, 1.0f, 1.0f };
    }

    uint32_t latticeToId(const ByteLattice &p) const {
        int ix = (int)((p.x + 1.0f) * 2048.0f);
        int iy = (int)((p.y + 1.0f) * 2048.0f);
        ix = std::clamp(ix, 0, 0xFFF);
        iy = std::clamp(iy, 0, 0xFFF);
        return (uint32_t)(ix | (iy << 12));
    }

    std::unique_ptr<TrieNode> m_root;
    std::unordered_map<uint32_t, std::string> m_vocab;
    std::unordered_map<uint32_t, float> m_scores;
};

} // namespace job::ai::token
