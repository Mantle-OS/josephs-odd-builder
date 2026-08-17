#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "algo/itoken_algo.h"
#include "core/trie.h"
#include "jobtoken_export.h"

namespace job::token {

// Hope you like the ######## symbol ##lo ##l

class JOBTOKEN_EXPORT Wordpiece final : public ITokenAlgo
{
public:
    using Ptr  = std::shared_ptr<Wordpiece>;
    using WPtr = std::weak_ptr<Wordpiece>;
    using UPtr = std::unique_ptr<Wordpiece>;

    explicit Wordpiece(const Vocab *vocab, std::string prefix = "##", std::size_t maxWordChars = 200);

    ~Wordpiece() override = default;

    Wordpiece(const Wordpiece &) = delete;
    Wordpiece &operator=(const Wordpiece &) = delete;
    Wordpiece(Wordpiece &&) = delete;
    Wordpiece &operator=(Wordpiece &&) = delete;

    [[nodiscard]] static UPtr createUniq(const Vocab *vocab, std::string prefix = "##", std::size_t maxWordChars = 200)
    {
        return std::make_unique<Wordpiece>(vocab, std::move(prefix), maxWordChars);
    }

    [[nodiscard]] static Ptr createShared(const Vocab *vocab, std::string prefix = "##", std::size_t maxWordChars = 200)
    {
        return std::make_shared<Wordpiece>(vocab, std::move(prefix), maxWordChars);
    }

    [[nodiscard]] TokenType type() const noexcept override
    {
        return TokenType::WordPiece;
    }

    void rebuildTries();
    void setPrefix(std::string prefix);

    [[nodiscard]] const std::string &prefix() const noexcept
    {
        return m_prefix;
    }

    void setMaxWordChars(std::size_t maxChars) noexcept
    {
        m_maxWordChars = maxChars;
    }

    [[nodiscard]] std::size_t maxWordChars() const noexcept
    {
        return m_maxWordChars;
    }

    std::size_t encode(std::string_view chunk, std::span<TokenId> outTokens) const override;
    std::size_t decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const override;

private:
    std::string m_prefix{"##"};
    std::size_t m_maxWordChars{200};

    Trie m_rootTrie;
    Trie m_continuationTrie;
};

} // namespace job::token