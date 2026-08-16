#pragma once

#include <string_view>
#include <vector>
#include <cstddef>

#include "template/jinja_token.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT JinjaLexer {
public:
    explicit JinjaLexer(std::string_view source);
    [[nodiscard]] JinjaToken nextToken();
    [[nodiscard]] std::vector<JinjaToken> tokenizeAll();
    void reset(std::string_view source) noexcept;

private:
    enum class LexerState : uint8_t {
        Text,   // Outside tags (scanning raw template text)
        InTag   // Inside {{ ... }} or {% ... %}
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

    std::string_view m_source;
    size_t           m_cursor{0};
    size_t           m_line{1};
    size_t           m_column{1};

    LexerState       m_state{LexerState::Text};
    bool             m_stripLeadingWhitespace{false};
};

} // namespace job::token