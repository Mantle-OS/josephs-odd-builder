#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

#include "job_token_types.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT Trie
{
public:
    using Ptr  = std::shared_ptr<Trie>;
    using WPtr = std::weak_ptr<Trie>;
    using UPtr = std::unique_ptr<Trie>;

    struct Match
    {
        TokenId id{kInvalidToken};
        std::size_t length{0};
    };

    Trie()
    {
        m_nodes.emplace_back();
    }

    ~Trie() = default;

    Trie(const Trie &) = default;
    Trie &operator=(const Trie &) = default;
    Trie(Trie &&) noexcept = default;
    Trie &operator=(Trie &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<Trie>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<Trie>();
    }

    void insert(std::string_view key, TokenId id)
    {
        if (key.empty() || id == kInvalidToken)
            return;

        std::uint32_t current = 0;
        for (const char c : key) {
            const std::uint8_t byte = static_cast<std::uint8_t>(c);
            std::uint32_t next = findChild(current, byte);

            if (next == kInvalidNode) {
                next = static_cast<std::uint32_t>(m_nodes.size());
                m_nodes.emplace_back();

                auto &children = m_nodes[current].children;
                auto it = std::lower_bound(children.begin(), children.end(), byte, [](const Child &edge, std::uint8_t value) {
                    return edge.byte < value;
                });

                children.insert(it, Child{byte, next});
            }

            current = next;
        }

        m_nodes[current].tokenId = id;
    }

    void clear()
    {
        m_nodes.clear();
        m_nodes.emplace_back();
    }

    [[nodiscard]] TokenId find(std::string_view key) const noexcept
    {
        std::uint32_t current = 0;

        for (const char c : key) {
            current = findChild(current, static_cast<std::uint8_t>(c));

            if (current == kInvalidNode)
                return kInvalidToken;
        }

        return m_nodes[current].tokenId;
    }

    [[nodiscard]] Match longestPrefix(std::string_view text) const noexcept
    {
        Match best{};
        std::uint32_t current = 0;

        for (std::size_t i = 0; i < text.size(); ++i) {
            current = findChild(current, static_cast<std::uint8_t>(text[i]));

            if (current == kInvalidNode)
                break;

            if (m_nodes[current].tokenId != kInvalidToken)
                best = Match{m_nodes[current].tokenId, i + 1};
        }

        return best;
    }

    // Appends all matching prefixes to the caller-owned buffer.
    void findAllPrefixes(std::string_view text, std::vector<Match> &outMatches) const
    {
        std::uint32_t current = 0;

        for (std::size_t i = 0; i < text.size(); ++i) {
            current = findChild(current, static_cast<std::uint8_t>(text[i]));

            if (current == kInvalidNode)
                break;

            if (m_nodes[current].tokenId != kInvalidToken)
                outMatches.push_back(Match{m_nodes[current].tokenId, i + 1});
        }
    }

    [[nodiscard]] std::size_t nodeCount() const noexcept { return m_nodes.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_nodes.size() <= 1; }

private:
    static constexpr std::uint32_t kInvalidNode =
        std::numeric_limits<std::uint32_t>::max();

    struct Child
    {
        std::uint8_t byte{0};
        std::uint32_t nodeIndex{kInvalidNode};
    };

    struct Node
    {
        TokenId tokenId{kInvalidToken};
        std::vector<Child> children;
    };

    [[nodiscard]] std::uint32_t findChild(
        std::uint32_t nodeIndex,
        std::uint8_t byte) const noexcept
    {
        if (nodeIndex >= m_nodes.size())
            return kInvalidNode;

        const auto &children = m_nodes[nodeIndex].children;

        auto it = std::lower_bound( children.begin(), children.end(), byte, [](const Child &edge, std::uint8_t value) {
            return edge.byte < value;
        });

        if (it != children.end() && it->byte == byte)
            return it->nodeIndex;

        return kInvalidNode;
    }

private:
    std::vector<Node> m_nodes;
};

} // namespace job::token