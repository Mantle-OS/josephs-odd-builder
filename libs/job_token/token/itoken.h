#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "job_token_enums.h"
#include "job_token_types.h"
#include "vocab/vocab.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT IToken
{
public:
    using Ptr  = std::shared_ptr<IToken>;
    using WPtr = std::weak_ptr<IToken>;
    using UPtr = std::unique_ptr<IToken>;

    enum class Provider : std::uint8_t {
        HuggingFace = 0,
        Gguf,
        Binary,
        Unknown
    };

    IToken() :
        m_vocab{Vocab::createShared()}
    {
    }

    virtual ~IToken() = default;

    IToken(const IToken &) = delete;
    IToken &operator=(const IToken &) = delete;
    IToken(IToken &&) = delete;
    IToken &operator=(IToken &&) = delete;

    // Provider ---------------------------------------------------------------

    [[nodiscard]] Provider provider() const noexcept
    {
        return m_provider;
    }

    void setProvider(Provider provider) noexcept
    {
        m_provider = provider;
    }

    // Token algorithm --------------------------------------------------------

    [[nodiscard]] TokenType tokenType() const noexcept
    {
        return m_tokenType;
    }

    void setTokenType(TokenType type) noexcept
    {
        m_tokenType = type;
    }

    // Vocabulary -------------------------------------------------------------

    [[nodiscard]] Vocab *vocab() noexcept
    {
        return m_vocab.get();
    }

    [[nodiscard]] const Vocab *vocab() const noexcept
    {
        return m_vocab.get();
    }

    [[nodiscard]] std::size_t vocabSize() const noexcept
    {
        return m_vocab ? m_vocab->size() : 0;
    }

    [[nodiscard]] SpecialTokens *specialTokens() noexcept
    {
        return m_vocab ? &m_vocab->specialTokens() : nullptr;
    }

    [[nodiscard]] const SpecialTokens *specialTokens() const noexcept
    {
        return m_vocab ? &m_vocab->specialTokens() : nullptr;
    }

    [[nodiscard]] decltype(auto) records() noexcept
    {
        return m_vocab->records();
    }

    [[nodiscard]] decltype(auto) records() const noexcept
    {
        return m_vocab->records();
    }

    // Vocabulary lookup shortcuts -------------------------------------------

    [[nodiscard]] std::optional<TokenId> findTokenId(std::string_view token) const noexcept
    {
        if (!m_vocab)
            return std::nullopt;

        const TokenId id = m_vocab->findId(token);

        if (id == kInvalidToken)
            return std::nullopt;

        return id;
    }

    [[nodiscard]] std::optional<std::string_view> findTokenString(TokenId id) const noexcept
    {
        if (!m_vocab)
            return std::nullopt;

        const std::string_view text = m_vocab->tokenText(id);

        if (text.empty())
            return std::nullopt;

        return text;
    }

    // Pre-tokenization configuration ----------------------------------------

    [[nodiscard]] SplitPattern splitPattern() const noexcept
    {
        return m_splitPattern;
    }

    void setSplitPattern(SplitPattern pattern) noexcept
    {
        m_splitPattern = pattern;
    }

    [[nodiscard]] const std::string &customSplitPattern() const noexcept
    {
        return m_customSplitPattern;
    }

    void setCustomSplitPattern(std::string pattern)
    {
        m_customSplitPattern = std::move(pattern);
        m_splitPattern = SplitPattern::Custom;
    }

    [[nodiscard]] bool addPrefixSpace() const noexcept
    {
        return m_addPrefixSpace;
    }

    void setAddPrefixSpace(bool enabled) noexcept
    {
        m_addPrefixSpace = enabled;
    }

    [[nodiscard]] bool byteFallback() const noexcept
    {
        return m_byteFallback;
    }

    void setByteFallback(bool enabled) noexcept
    {
        m_byteFallback = enabled;
    }

    // Sequence configuration -------------------------------------------------

    [[nodiscard]] bool addBosToken() const noexcept
    {
        return m_addBosToken;
    }

    void setAddBosToken(bool enabled) noexcept
    {
        m_addBosToken = enabled;
    }

    [[nodiscard]] bool addEosToken() const noexcept
    {
        return m_addEosToken;
    }

    void setAddEosToken(bool enabled) noexcept
    {
        m_addEosToken = enabled;
    }

    // Chat configuration -----------------------------------------------------

    [[nodiscard]] const std::string &chatTemplate() const noexcept
    {
        return m_chatTemplate;
    }

    void setChatTemplate(std::string chatTemplate)
    {
        m_chatTemplate = std::move(chatTemplate);
    }

    // State ------------------------------------------------------------------

    virtual void clear() noexcept
    {
        m_provider = Provider::Unknown;
        m_tokenType = TokenType::Unknown;

        m_splitPattern = SplitPattern::None;
        m_customSplitPattern.clear();
        m_addPrefixSpace = false;
        m_byteFallback = false;

        m_addBosToken = false;
        m_addEosToken = false;

        m_chatTemplate.clear();

        if (m_vocab)
            m_vocab->clear();

        extraClear();
    }

protected:
    [[nodiscard]] static std::string readFile(const std::filesystem::path &path)
    {
        std::ifstream file{path, std::ios::binary};

        if (!file.is_open())
            return {};

        std::ostringstream stream;
        stream << file.rdbuf();

        return stream.str();
    }
    virtual void extraClear() noexcept = 0;


private:
    // Loaded tokenizer identity.
    Provider        m_provider{Provider::Unknown};
    TokenType   m_tokenType{TokenType::Unknown};

    // Canonical token data.
    Vocab::Ptr  m_vocab;

    // Description consumed by JobTokenizer when constructing
    // its runtime pre-tokenization pipeline.
    SplitPattern    m_splitPattern{SplitPattern::None};
    std::string     m_customSplitPattern;
    bool            m_addPrefixSpace{false};
    bool            m_byteFallback{false};

    // Sequence behavior applied around encoded token streams.
    bool    m_addBosToken{false};
    bool    m_addEosToken{false};

    // Chat template source consumed by ChatEngine at runtime.
    std::string     m_chatTemplate;
};

} // namespace job::token