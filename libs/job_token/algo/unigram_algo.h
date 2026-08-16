#pragma once

#include <cstdint>
#include <string_view>
#include <span>
#include <vector>
#include <memory>

#include <real_type.h>
#include "algo/itoken_algo.h"

#include "core/trie.h"

#include "core/byte_fallback.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT UnigramAlgo final : public ITokenAlgo {
public:
    using Ptr  = std::shared_ptr<UnigramAlgo>;
    using UPtr = std::unique_ptr<UnigramAlgo>;

    // Finite sentinel threshold for unreachable DP nodes under fast-math
    static constexpr float kUnreachableScore = -1e30f;
    static constexpr float kMinValidScore    = -1e20f;

    explicit UnigramAlgo(Vocab::Ptr vocab);
    ~UnigramAlgo() override = default;

    [[nodiscard]] static UPtr create(Vocab::Ptr vocab)
    {
        return std::make_unique<UnigramAlgo>(std::move(vocab));
    }

    [[nodiscard]] TokenizerAlgorithm type() const noexcept override
    {
        return TokenizerAlgorithm::Unigram;
    }

    // Rebuilds the internal prefix matching Trie from the active vocabulary
    void rebuildTrie();

    size_t encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const override;
    size_t decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const override;

    [[nodiscard]] const Trie& trie() const noexcept
    {
        return m_trie;
    }

private:
    struct DpNode {
        float    bestScore{kUnreachableScore};
        TokenId  bestToken{kInvalidToken};
        uint32_t bestPrev{0};
    };

    [[nodiscard]] TokenId findByteFallbackToken(uint8_t byteVal) const noexcept;

    Trie m_trie;
    float m_unkScore{-10.0f};
};

} // namespace job::token