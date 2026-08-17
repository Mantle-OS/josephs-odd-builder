#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "job_token_enums.h"
#include "job_token_types.h"
#include "vocab/vocab.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ITokenAlgo
{
public:
    using Ptr  = std::shared_ptr<ITokenAlgo>;
    using WPtr = std::weak_ptr<ITokenAlgo>;
    using UPtr = std::unique_ptr<ITokenAlgo>;

    explicit ITokenAlgo(const Vocab *vocab) noexcept :
        m_vocab{vocab}
    {
    }

    virtual ~ITokenAlgo() = default;

    ITokenAlgo(const ITokenAlgo &) = delete;
    ITokenAlgo &operator=(const ITokenAlgo &) = delete;
    ITokenAlgo(ITokenAlgo &&) = delete;
    ITokenAlgo &operator=(ITokenAlgo &&) = delete;

    [[nodiscard]] virtual TokenType type() const noexcept = 0;

    virtual std::size_t encode(std::string_view text, std::span<TokenId> outTokens) const = 0;

    [[nodiscard]] virtual std::vector<TokenId> encode(std::string_view text) const
    {
        std::vector<TokenId> result(text.size() * 2 + 16);

        const std::size_t written = encode(text, result);
        result.resize(written);

        return result;
    }
    [[nodiscard]] virtual std::vector<TokenId> encode(std::span<const std::string> symbols) const
    {
        std::size_t size = 0;
        for (const std::string &symbol : symbols)
            size += symbol.size();

        std::string text;
        text.reserve(size);

        for (const std::string &symbol : symbols)
            text += symbol;

        return encode(text);
    }

    virtual std::size_t decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const = 0;
    [[nodiscard]] virtual std::string decode(std::span<const TokenId> tokens) const
    {
        std::string result;
        result.resize(tokens.size() * 16 + 64);
        const std::size_t written = decode(tokens,std::span<char>{ result.data(), result.size() });

        result.resize(written);
        return result;
    }
    [[nodiscard]] virtual ByteSymbols decodeSymbols(std::span<const TokenId> tokens) const
    {
        ByteSymbols symbols;
        symbols.reserve(tokens.size());

        if (!m_vocab)
            return symbols;

        for (const TokenId id : tokens) {
            const std::string_view text = m_vocab->tokenText(id);
            if (!text.empty())
                symbols.emplace_back(text);
        }

        return symbols;
    }

    [[nodiscard]] const Vocab *vocab() const noexcept
    {
        return m_vocab;
    }

    void setVocab(const Vocab *vocab) noexcept
    {
        m_vocab = vocab;
    }

protected:
    const Vocab *m_vocab{nullptr};
};

} // namespace job::token