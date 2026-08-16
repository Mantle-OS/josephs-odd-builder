#pragma once

#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <memory>

#include "job_tokenizer_types.h"
#include "vocab/vocab.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ITokenAlgo {
public:
    using Ptr  = std::shared_ptr<ITokenAlgo>;
    using UPtr = std::unique_ptr<ITokenAlgo>;

    explicit ITokenAlgo(Vocab::Ptr vocab) :
        m_vocab{std::move(vocab)}
    {
    }

    virtual ~ITokenAlgo() = default;

    ITokenAlgo(const ITokenAlgo&) = default;
    ITokenAlgo& operator=(const ITokenAlgo&) = default;
    ITokenAlgo(ITokenAlgo&&) noexcept = default;
    ITokenAlgo& operator=(ITokenAlgo&&) noexcept = default;

    [[nodiscard]] virtual TokenizerAlgorithm type() const noexcept = 0;

    // Non-allocating chunk encode. Writes token IDs to outTokens.
    // Returns the number of tokens written.
    virtual size_t encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const = 0;

    // Allocating chunk encode helper
    [[nodiscard]] virtual std::vector<TokenId> encodeChunk(std::string_view chunk) const
    {
        std::vector<TokenId> result(chunk.size() * 2 + 16);
        const size_t written = encodeChunk(chunk, result);
        result.resize(written);
        return result;
    }

    // Non-allocating token sequence decode. Writes characters/bytes to outBuffer.
    // Returns the number of bytes written.
    virtual size_t decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const = 0;

    // Allocating token sequence decode helper
    [[nodiscard]] virtual std::string decodeTokens(std::span<const TokenId> tokens) const
    {
        std::string result;
        result.resize(tokens.size() * 16 + 64);
        const size_t written = decodeTokens(tokens, std::span<char>(result.data(), result.size()));
        result.resize(written);
        return result;
    }

    [[nodiscard]] Vocab* vocab() noexcept
    {
        return m_vocab.get();
    }
    [[nodiscard]] const Vocab* vocab() const noexcept
    {
        return m_vocab.get();
    }

    void setVocab(Vocab::Ptr vocab) noexcept
    {
        m_vocab = std::move(vocab);
    }

protected:
    Vocab::Ptr m_vocab;
};

} // namespace job::token