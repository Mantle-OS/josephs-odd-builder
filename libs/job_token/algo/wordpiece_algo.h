#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <memory>

#include "algo/itoken_algo.h"
#include "core/trie.h"
#include "core/byte_fallback.h"
#include "jobtoken_export.h"

namespace job::token {

// Hope you like the ######## symbol lol
class JOBTOKEN_EXPORT WordpieceAlgo final : public ITokenAlgo {
public:
    using Ptr  = std::shared_ptr<WordpieceAlgo>;
    using UPtr = std::unique_ptr<WordpieceAlgo>;

    explicit WordpieceAlgo(Vocab::Ptr vocab, std::string continuationPrefix = "##", size_t maxInputCharsPerWord = 200);
    ~WordpieceAlgo() override = default;

    [[nodiscard]] static UPtr create(Vocab::Ptr vocab, std::string continuationPrefix = "##", size_t maxInputCharsPerWord = 200)
    {
        return std::make_unique<WordpieceAlgo>(std::move(vocab), std::move(continuationPrefix), maxInputCharsPerWord);
    }

    [[nodiscard]] TokenizerAlgorithm type() const noexcept override
    {
        return TokenizerAlgorithm::WordPiece;
    }

    void rebuildTries();

    void setContinuationPrefix(std::string prefix);
    [[nodiscard]] const std::string& continuationPrefix() const noexcept
    {
        return m_continuationPrefix;
    }

    void setMaxInputCharsPerWord(size_t maxChars) noexcept
    {
        m_maxInputCharsPerWord = maxChars;
    }
    [[nodiscard]] size_t maxInputCharsPerWord() const noexcept
    {
        return m_maxInputCharsPerWord;
    }

    size_t encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const override;
    size_t decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const override;

private:
    std::string     m_continuationPrefix{"##"};
    size_t          m_maxInputCharsPerWord{200};
    Trie            m_rootTrie;
    Trie            m_continuationTrie;
};

} // namespace job::token