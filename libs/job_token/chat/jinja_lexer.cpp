#include "jinja_lexer.h"

namespace job::token {

JinjaLexer::JinjaLexer(std::string_view source)
    : m_source{source}
{
}

void JinjaLexer::reset(std::string_view source) noexcept
{
    m_source = source;
    m_cursor = 0;
    m_line = 1;
    m_column = 1;
    m_state = LexerState::Text;
    m_stripLeadingWhitespace = false;
}

char JinjaLexer::peek() const noexcept
{
    if (atEnd())
        return '\0';

    return m_source[m_cursor];
}

char JinjaLexer::peekNext() const noexcept
{
    if (m_cursor + 1 >= m_source.size())
        return '\0';

    return m_source[m_cursor + 1];
}

char JinjaLexer::advance() noexcept
{
    if (atEnd())
        return '\0';

    const char c = m_source[m_cursor++];

    if (c == '\n') {
        ++m_line;
        m_column = 1;
    }
    else {
        ++m_column;
    }

    return c;
}

std::vector<JinjaToken> JinjaLexer::tokenizeAll()
{
    std::vector<JinjaToken> tokens;
    tokens.reserve(m_source.size() / 8 + 16);

    while (true) {
        JinjaToken token = nextToken();
        tokens.push_back(token);

        if (token.type == JinjaType::Eof)
            break;
    }

    return tokens;
}

JinjaToken JinjaLexer::nextToken()
{
    while (!atEnd()) {
        if (m_state == LexerState::Text)
            return scanText();

        return scanTagToken();
    }

    return JinjaToken{
        JinjaType::Eof,
        {},
        m_line,
        m_column
    };
}

void JinjaLexer::skipComment() noexcept
{
    while (!atEnd()) {
        if (peek() == '-' &&
            m_cursor + 2 < m_source.size() &&
            m_source[m_cursor + 1] == '#' &&
            m_source[m_cursor + 2] == '}') {

            advance(); // -
            advance(); // #
            advance(); // }

            m_stripLeadingWhitespace = true;
            return;
        }

        if (peek() == '#' && peekNext() == '}') {
            advance(); // #
            advance(); // }

            m_stripLeadingWhitespace = false;
            return;
        }

        advance();
    }
}

JinjaToken JinjaLexer::scanText()
{
    if (m_stripLeadingWhitespace) {
        while (!atEnd() && isWhitespace(peek()))
            advance();

        m_stripLeadingWhitespace = false;
    }

    std::size_t startLine = m_line;
    std::size_t startColumn = m_column;
    std::size_t textStart = m_cursor;

    while (!atEnd()) {
        if (peek() != '{') {
            advance();
            continue;
        }

        const char next = peekNext();

        if (next != '{' && next != '%' && next != '#') {
            advance();
            continue;
        }

        const std::size_t tagPos = m_cursor;
        const std::size_t tagLine = m_line;
        const std::size_t tagColumn = m_column;

        const bool trimBefore =
            m_cursor + 2 < m_source.size() &&
            m_source[m_cursor + 2] == '-';

        //
        // ------------------------------------------------------------
        // Comment
        // ------------------------------------------------------------
        //
        if (next == '#') {
            std::string_view chunk =
                m_source.substr(textStart, tagPos - textStart);

            if (trimBefore)
                chunk = trimTrailingWhitespace(chunk);

            advance(); // {
            advance(); // #

            if (trimBefore)
                advance(); // -

            skipComment();

            //
            // Return any text that appeared before the comment.
            // The comment itself has already been consumed.
            //
            if (!chunk.empty()) {
                return JinjaToken{
                    JinjaType::Text,
                    chunk,
                    startLine,
                    startColumn
                };
            }

            //
            // A -#} comment closing strips whitespace following
            // the comment before raw-text scanning resumes.
            //
            if (m_stripLeadingWhitespace) {
                while (!atEnd() && isWhitespace(peek()))
                    advance();

                m_stripLeadingWhitespace = false;
            }

            //
            // The old text start was before the comment. Reset it so
            // the consumed comment can never become part of Text.
            //
            textStart = m_cursor;
            startLine = m_line;
            startColumn = m_column;

            continue;
        }

        //
        // ------------------------------------------------------------
        // Expression / block opening
        // ------------------------------------------------------------
        //
        if (tagPos > textStart) {
            std::string_view chunk =
                m_source.substr(textStart, tagPos - textStart);

            if (trimBefore)
                chunk = trimTrailingWhitespace(chunk);

            if (!chunk.empty()) {
                return JinjaToken{
                    JinjaType::Text,
                    chunk,
                    startLine,
                    startColumn
                };
            }
        }

        //
        // No text remains before the tag, so consume the opening
        // delimiter now.
        //
        if (next == '{') {
            m_state = LexerState::Expression;

            advance(); // {
            advance(); // {

            if (trimBefore)
                advance(); // -

            return JinjaToken{
                JinjaType::ExprBegin,
                m_source.substr(tagPos, m_cursor - tagPos),
                tagLine,
                tagColumn
            };
        }

        m_state = LexerState::Block;

        advance(); // {
        advance(); // %

        if (trimBefore)
            advance(); // -

        return JinjaToken{
            JinjaType::BlockBegin,
            m_source.substr(tagPos, m_cursor - tagPos),
            tagLine,
            tagColumn
        };
    }

    if (m_cursor > textStart) {
        return JinjaToken{
            JinjaType::Text,
            m_source.substr(textStart, m_cursor - textStart),
            startLine,
            startColumn
        };
    }

    return JinjaToken{
        JinjaType::Eof,
        {},
        m_line,
        m_column
    };
}

void JinjaLexer::skipWhitespaceInsideTag() noexcept
{
    while (!atEnd() && isWhitespace(peek()))
        advance();
}

JinjaToken JinjaLexer::scanTagToken()
{
    skipWhitespaceInsideTag();

    if (atEnd()) {
        //
        // EOF while still inside a tag means the template ended
        // before its closing delimiter.
        //
        return JinjaToken{
            JinjaType::Unknown,
            {},
            m_line,
            m_column
        };
    }

    const std::size_t startLine = m_line;
    const std::size_t startColumn = m_column;
    const std::size_t startPos = m_cursor;
    const char c = peek();

    //
    // ------------------------------------------------------------
    // Expression closing
    // ------------------------------------------------------------
    //
    if (m_state == LexerState::Expression) {
        if (c == '-' &&
            peekNext() == '}' &&
            m_cursor + 2 < m_source.size() &&
            m_source[m_cursor + 2] == '}') {

            advance(); // -
            advance(); // }
            advance(); // }

            m_state = LexerState::Text;
            m_stripLeadingWhitespace = true;

            return JinjaToken{
                JinjaType::ExprEnd,
                m_source.substr(startPos, 3),
                startLine,
                startColumn
            };
        }

        if (c == '}' && peekNext() == '}') {
            advance(); // }
            advance(); // }

            m_state = LexerState::Text;
            m_stripLeadingWhitespace = false;

            return JinjaToken{
                JinjaType::ExprEnd,
                m_source.substr(startPos, 2),
                startLine,
                startColumn
            };
        }
    }

    //
    // ------------------------------------------------------------
    // Block closing
    // ------------------------------------------------------------
    //
    if (m_state == LexerState::Block) {
        if (c == '-' &&
            peekNext() == '%' &&
            m_cursor + 2 < m_source.size() &&
            m_source[m_cursor + 2] == '}') {

            advance(); // -
            advance(); // %
            advance(); // }

            m_state = LexerState::Text;
            m_stripLeadingWhitespace = true;

            return JinjaToken{
                JinjaType::BlockEnd,
                m_source.substr(startPos, 3),
                startLine,
                startColumn
            };
        }

        if (c == '%' && peekNext() == '}') {
            advance(); // %
            advance(); // }

            m_state = LexerState::Text;
            m_stripLeadingWhitespace = false;

            return JinjaToken{
                JinjaType::BlockEnd,
                m_source.substr(startPos, 2),
                startLine,
                startColumn
            };
        }
    }

    //
    // ------------------------------------------------------------
    // String literal
    // ------------------------------------------------------------
    //
    if (c == '"' || c == '\'')
        return scanString(c);

    //
    // ------------------------------------------------------------
    // Number literal
    // ------------------------------------------------------------
    //
    if (c >= '0' && c <= '9')
        return scanNumber();

    //
    // ------------------------------------------------------------
    // Identifier / keyword
    // ------------------------------------------------------------
    //
    if (isIdentStart(c))
        return scanIdentifierOrKeyword();

    //
    // ------------------------------------------------------------
    // Multi-character operators
    // ------------------------------------------------------------
    //
    if (c == '=' && peekNext() == '=') {
        advance();
        advance();

        return JinjaToken{
            JinjaType::Eq,
            m_source.substr(startPos, 2),
            startLine,
            startColumn
        };
    }

    if (c == '!' && peekNext() == '=') {
        advance();
        advance();

        return JinjaToken{
            JinjaType::Ne,
            m_source.substr(startPos, 2),
            startLine,
            startColumn
        };
    }

    if (c == '<' && peekNext() == '=') {
        advance();
        advance();

        return JinjaToken{
            JinjaType::Le,
            m_source.substr(startPos, 2),
            startLine,
            startColumn
        };
    }

    if (c == '>' && peekNext() == '=') {
        advance();
        advance();

        return JinjaToken{
            JinjaType::Ge,
            m_source.substr(startPos, 2),
            startLine,
            startColumn
        };
    }

    if (c == '/' && peekNext() == '/') {
        advance();
        advance();

        return JinjaToken{
            JinjaType::FloorDiv,
            m_source.substr(startPos, 2),
            startLine,
            startColumn
        };
    }

    if (c == '*' && peekNext() == '*') {
        advance();
        advance();

        return JinjaToken{
            JinjaType::Pow,
            m_source.substr(startPos, 2),
            startLine,
            startColumn
        };
    }

    //
    // ------------------------------------------------------------
    // Single-character operators / punctuation
    // ------------------------------------------------------------
    //
    advance();

    switch (c) {
    case '.':
        return JinjaToken{
            JinjaType::Dot,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case ',':
        return JinjaToken{
            JinjaType::Comma,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case ':':
        return JinjaToken{
            JinjaType::Colon,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case ';':
        return JinjaToken{
            JinjaType::Semicolon,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '|':
        return JinjaToken{
            JinjaType::Pipe,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '~':
        return JinjaToken{
            JinjaType::Tilde,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '+':
        return JinjaToken{
            JinjaType::Plus,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '-':
        return JinjaToken{
            JinjaType::Minus,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '*':
        return JinjaToken{
            JinjaType::Mul,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '/':
        return JinjaToken{
            JinjaType::Div,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '%':
        return JinjaToken{
            JinjaType::Mod,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '<':
        return JinjaToken{
            JinjaType::Lt,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '>':
        return JinjaToken{
            JinjaType::Gt,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '=':
        return JinjaToken{
            JinjaType::Assign,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '(':
        return JinjaToken{
            JinjaType::ParenOpen,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case ')':
        return JinjaToken{
            JinjaType::ParenClose,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '[':
        return JinjaToken{
            JinjaType::BracketOpen,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case ']':
        return JinjaToken{
            JinjaType::BracketClose,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '{':
        return JinjaToken{
            JinjaType::BraceOpen,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    case '}':
        return JinjaToken{
            JinjaType::BraceClose,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };

    default:
        return JinjaToken{
            JinjaType::Unknown,
            m_source.substr(startPos, 1),
            startLine,
            startColumn
        };
    }
}

JinjaToken JinjaLexer::scanString(char quote)
{
    const std::size_t startLine = m_line;
    const std::size_t startColumn = m_column;

    advance(); // Opening quote

    const std::size_t contentStart = m_cursor;

    while (!atEnd()) {
        const char c = peek();
        if (c == '\\') {
            advance();

            if (!atEnd())
                advance();

            continue;
        }

        if (c == quote) {
            const std::size_t contentEnd = m_cursor;

            advance(); // Closing quote

            return JinjaToken{
                JinjaType::StringLiteral,
                m_source.substr(
                    contentStart,
                    contentEnd - contentStart),
                startLine,
                startColumn
            };
        }

        advance();
    }

    //
    // Opening quote without a matching closing quote.
    //
    return JinjaToken{
        JinjaType::Unknown,
        m_source.substr(contentStart),
        startLine,
        startColumn
    };
}

JinjaToken JinjaLexer::scanNumber()
{
    const std::size_t startLine = m_line;
    const std::size_t startColumn = m_column;
    const std::size_t startPos = m_cursor;

    bool hasDecimal = false;

    while (!atEnd()) {
        const char c = peek();

        if (c >= '0' && c <= '9') {
            advance();
            continue;
        }

        if (c == '.' && !hasDecimal) {
            hasDecimal = true;
            advance();
            continue;
        }

        break;
    }

    return JinjaToken{
        JinjaType::NumberLiteral,
        m_source.substr(startPos, m_cursor - startPos),
        startLine,
        startColumn
    };
}

JinjaToken JinjaLexer::scanIdentifierOrKeyword()
{
    const std::size_t startLine = m_line;
    const std::size_t startColumn = m_column;
    const std::size_t startPos = m_cursor;

    while (!atEnd() && isIdentBody(peek()))
        advance();

    const std::string_view ident =
        m_source.substr(startPos, m_cursor - startPos);

    if (ident == "for")
        return JinjaToken{JinjaType::KwFor, ident, startLine, startColumn};

    if (ident == "in")
        return JinjaToken{JinjaType::KwIn, ident, startLine, startColumn};

    if (ident == "endfor")
        return JinjaToken{JinjaType::KwEndFor, ident, startLine, startColumn};

    if (ident == "if")
        return JinjaToken{JinjaType::KwIf, ident, startLine, startColumn};

    if (ident == "elif")
        return JinjaToken{JinjaType::KwElif, ident, startLine, startColumn};

    if (ident == "else")
        return JinjaToken{JinjaType::KwElse, ident, startLine, startColumn};

    if (ident == "endif")
        return JinjaToken{JinjaType::KwEndIf, ident, startLine, startColumn};

    if (ident == "set")
        return JinjaToken{JinjaType::KwSet, ident, startLine, startColumn};

    if (ident == "and")
        return JinjaToken{JinjaType::KwAnd, ident, startLine, startColumn};

    if (ident == "or")
        return JinjaToken{JinjaType::KwOr, ident, startLine, startColumn};

    if (ident == "not")
        return JinjaToken{JinjaType::KwNot, ident, startLine, startColumn};

    if (ident == "is")
        return JinjaToken{JinjaType::KwIs, ident, startLine, startColumn};

    if (ident == "true" || ident == "True") {
        return JinjaToken{
            JinjaType::KwTrue,
            ident,
            startLine,
            startColumn
        };
    }

    if (ident == "false" || ident == "False") {
        return JinjaToken{
            JinjaType::KwFalse,
            ident,
            startLine,
            startColumn
        };
    }

    if (ident == "none" ||
        ident == "None" ||
        ident == "null") {

        return JinjaToken{
            JinjaType::KwNone,
            ident,
            startLine,
            startColumn
        };
    }

    return JinjaToken{
        JinjaType::Identifier,
        ident,
        startLine,
        startColumn
    };
}

} // namespace job::token