#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "jinja_token.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT JinjaLexer
{
public:
    explicit JinjaLexer(std::string_view source);

    [[nodiscard]] JinjaToken nextToken();
    [[nodiscard]] std::vector<JinjaToken> tokenizeAll();

    void reset(std::string_view source) noexcept;

private:
    enum class LexerState : uint8_t {
        Text,
        Expression,
        Block
    };

    [[nodiscard]] JinjaToken scanText();
    [[nodiscard]] JinjaToken scanTagToken();
    [[nodiscard]] JinjaToken scanString(char quote);
    [[nodiscard]] JinjaToken scanNumber();
    [[nodiscard]] JinjaToken scanIdentifierOrKeyword();

    void skipWhitespaceInsideTag() noexcept;
    void skipComment() noexcept;

    [[nodiscard]] bool atEnd() const noexcept
    {
        return m_cursor >= m_source.size();
    }

    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peekNext() const noexcept;
    char advance() noexcept;

    [[nodiscard]] static constexpr bool isWhitespace(char c) noexcept
    {
        return c == ' ' ||
               c == '\t' ||
               c == '\n' ||
               c == '\r' ||
               c == '\f' ||
               c == '\v';
    }

    [[nodiscard]] static constexpr bool isIdentStart(char c) noexcept
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '_';
    }

    [[nodiscard]] static constexpr bool isIdentBody(char c) noexcept
    {
        return isIdentStart(c) ||
               (c >= '0' && c <= '9');
    }

    [[nodiscard]] static constexpr std::string_view trimTrailingWhitespace(std::string_view text) noexcept
    {
        while (!text.empty() && isWhitespace(text.back()))
            text.remove_suffix(1);

        return text;
    }

    std::string_view m_source;
    std::size_t      m_cursor{0};
    std::size_t      m_line{1};
    std::size_t      m_column{1};

    LexerState m_state{LexerState::Text};
    bool       m_stripLeadingWhitespace{false};
};

} // namespace job::token