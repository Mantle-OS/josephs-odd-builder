#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <limits>
#include <algorithm>

#include "job_tokenizer_types.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT Trie {
public:
    struct Match
    {
        TokenId id{kInvalidToken};
        size_t  length{0};
    };

    Trie()
    {
        // Reserve root node at index 0
        m_nodes.emplace_back();
    }

    ~Trie() = default;

    Trie(const Trie&) = default;
    Trie& operator=(const Trie&) = default;
    Trie(Trie&&) noexcept = default;
    Trie& operator=(Trie&&) noexcept = default;

    // --- Mutation ---
    void insert(std::string_view key, TokenId id)
    {
        if (key.empty() || id == kInvalidToken)
            return;

        uint32_t currIndex = 0;
        for (const char c : key) {
            const uint8_t byteVal = static_cast<uint8_t>(c);
            uint32_t nextIndex = findChild(currIndex, byteVal);

            if (nextIndex == kInvalidNode) {
                nextIndex = static_cast<uint32_t>(m_nodes.size());
                m_nodes.emplace_back();

                // Keep children sorted by byte for O(log K) binary search
                auto& children = m_nodes[currIndex].children;
                auto it = std::lower_bound(children.begin(), children.end(), byteVal,[](const Child& edge, uint8_t val) {
                    return edge.byteVal < val;
                });
                children.insert(it, Child{byteVal, nextIndex});
            }

            currIndex = nextIndex;
        }

        m_nodes[currIndex].tokenId = id;
    }

    void clear()
    {
        m_nodes.clear();
        m_nodes.emplace_back(); // Re-create root node
    }

    // --- Search Queries (Zero-Allocation) ---

    // Exact key lookup
    [[nodiscard]] TokenId find(std::string_view key) const noexcept
    {
        uint32_t currIndex = 0;
        for (const char c : key) {
            currIndex = findChild(currIndex, static_cast<uint8_t>(c));
            if (currIndex == kInvalidNode)
                return kInvalidToken;
        }
        return m_nodes[currIndex].tokenId;
    }

    // Finds the longest matching prefix starting at the beginning of text
    [[nodiscard]] Match longestPrefix(std::string_view text) const noexcept
    {
        Match bestMatch{};
        uint32_t currIndex = 0;

        for (size_t i = 0; i < text.size(); ++i) {
            currIndex = findChild(currIndex, static_cast<uint8_t>(text[i]));
            if (currIndex == kInvalidNode)
                break;

            if (m_nodes[currIndex].tokenId != kInvalidToken) {
                bestMatch.id = m_nodes[currIndex].tokenId;
                bestMatch.length = i + 1;
            }
        }

        return bestMatch;
    }

    // Finds all matching prefixes starting at the beginning of text (used in Viterbi lattices)
    void findAllPrefixes(std::string_view text, std::vector<Match>& outMatches) const
    {
        uint32_t currIndex = 0;

        for (size_t i = 0; i < text.size(); ++i) {
            currIndex = findChild(currIndex, static_cast<uint8_t>(text[i]));
            if (currIndex == kInvalidNode)
                break;

            if (m_nodes[currIndex].tokenId != kInvalidToken)
                outMatches.push_back(Match{m_nodes[currIndex].tokenId, i + 1});
        }
    }

    [[nodiscard]] size_t nodeCount() const noexcept { return m_nodes.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_nodes.size() <= 1; }

private:
    static constexpr uint32_t kInvalidNode = std::numeric_limits<uint32_t>::max();

    struct Child
    {
        uint8_t  byteVal{0};
        uint32_t nodeIndex{kInvalidNode};
    };

    struct Node
    {
        TokenId            tokenId{kInvalidToken};
        std::vector<Child> children;
    };

    [[nodiscard]] uint32_t findChild(uint32_t nodeIndex, uint8_t byteVal) const noexcept
    {
        if (nodeIndex >= m_nodes.size())
            return kInvalidNode;

        const auto& children = m_nodes[nodeIndex].children;
        auto it = std::lower_bound(children.begin(), children.end(), byteVal, [](const Child& edge, uint8_t val) {
            return edge.byteVal < val;
        });

        if (it != children.end() && it->byteVal == byteVal)
            return it->nodeIndex;

        return kInvalidNode;
    }

    std::vector<Node> m_nodes;
};

} // namespace job::token