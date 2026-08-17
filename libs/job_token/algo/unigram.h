#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include <real_type.h>

#include "algo/itoken_algo.h"
#include "core/trie.h"

#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT Unigram final : public ITokenAlgo
{
public:
    using Ptr  = std::shared_ptr<Unigram>;
    using WPtr = std::weak_ptr<Unigram>;
    using UPtr = std::unique_ptr<Unigram>;

    // Finite sentinel thresholds for unreachable DP nodes under fast-math.
    static constexpr float kUnreachableScore = -1e30f;
    static constexpr float kMinValidScore     = -1e20f;

    explicit Unigram(const Vocab *vocab) noexcept;
    ~Unigram() override = default;

    Unigram(const Unigram &) = delete;
    Unigram &operator=(const Unigram &) = delete;
    Unigram(Unigram &&) = delete;
    Unigram &operator=(Unigram &&) = delete;

    [[nodiscard]] static UPtr createUniq(const Vocab *vocab)
    {
        return std::make_unique<Unigram>(vocab);
    }

    [[nodiscard]] static Ptr createShared(const Vocab *vocab)
    {
        return std::make_shared<Unigram>(vocab);
    }

    [[nodiscard]] TokenType type() const noexcept override
    {
        return TokenType::Unigram;
    }

    // Rebuilds the internal prefix-matching trie from the active vocabulary.
    void rebuildTrie();

    std::size_t encode(std::string_view chunk, std::span<TokenId> outTokens) const override;
    std::size_t decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const override;

    [[nodiscard]] const Trie &trie() const noexcept
    {
        return m_trie;
    }

private:
    struct DpNode
    {
        float bestScore{kUnreachableScore};
        TokenId bestToken{kInvalidToken};
        std::size_t bestPrev{0};
    };

    // UNRESOLVED:
    // Byte-fallback policy probably belongs above the Unigram algorithm layer.
    // Keep this temporarily while ownership/policy boundaries are being sorted.
    [[nodiscard]] TokenId findByteFallbackToken(std::uint8_t byte) const noexcept;

    Trie m_trie;
    float m_unkScore{-10.0f};
};

} // namespace job::token