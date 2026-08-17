#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

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

    virtual std::size_t decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const = 0;
    [[nodiscard]] virtual std::string decode(std::span<const TokenId> tokens) const
    {
        std::string result;
        result.resize(tokens.size() * 16 + 64);
        const std::size_t written = decode(tokens, std::span<char>{result.data(), result.size()});
        result.resize(written);
        return result;
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