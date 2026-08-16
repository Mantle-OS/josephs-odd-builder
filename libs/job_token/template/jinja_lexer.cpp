#include "template/jinja_lexer.h"

#include "template/jinja_utils.h"
#include <cctype>

namespace job::token {

JinjaLexer::JinjaLexer(std::string_view source) : m_source{source}
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
    if (atEnd()) return '\0';
    return m_source[m_cursor];
}

char JinjaLexer::peekNext() const noexcept
{
    if (m_cursor + 1 >= m_source.size()) return '\0';
    return m_source[m_cursor + 1];
}

char JinjaLexer::advance() noexcept
{
    if (atEnd()) return '\0';
    const char c = m_source[m_cursor++];
    if (c == '\n') {
        m_line += 1;
        m_column = 1;
    } else {
        m_column += 1;
    }
    return c;
}

std::vector<JinjaToken> JinjaLexer::tokenizeAll()
{
    std::vector<JinjaToken> tokens;
    tokens.reserve(m_source.size() / 8 + 16);

    while (true) {
        JinjaToken tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == JinjaTokenType::Eof) {
            break;
        }
    }

    return tokens;
}

JinjaToken JinjaLexer::nextToken()
{
    while (!atEnd()) {
        if (m_state == LexerState::Text) {
            return scanText();
        }
        return scanTagToken();
    }

    return JinjaToken{JinjaTokenType::Eof, {}, m_line, m_column};
}

void JinjaLexer::skipComment() noexcept
{
    // Scans through comment body until '#}' or '-#}'
    while (!atEnd()) {
        if (peek() == '-' && m_cursor + 2 < m_source.size() && m_source[m_cursor + 1] == '#' && m_source[m_cursor + 2] == '}') {
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
    const size_t startLine = m_line;
    const size_t startCol  = m_column;

    if (m_stripLeadingWhitespace) {
        while (!atEnd() && std::isspace(static_cast<unsigned char>(peek()))) {
            advance();
        }
        m_stripLeadingWhitespace = false;
    }

    const size_t textStart = m_cursor;

    while (!atEnd()) {
        // Check for tag opening
        if (peek() == '{') {
            const char next = peekNext();
            if (next == '{' || next == '%' || next == '#') {
                // If comment, skip it directly and continue text scanning
                if (next == '#') {
                    const size_t tagPos = m_cursor;
                    bool trimBefore = (m_cursor + 2 < m_source.size() && m_source[m_cursor + 2] == '-');

                    if (tagPos > textStart) {
                        std::string_view chunk = m_source.substr(textStart, tagPos - textStart);
                        if (trimBefore) {
                            chunk = utils::trimTrailingWhitespace(chunk);
                        }
                        advance(); // {
                        advance(); // #
                        if (trimBefore) advance(); // -
                        skipComment();
                        if (!chunk.empty()) {
                            return JinjaToken{JinjaTokenType::Text, chunk, startLine, startCol};
                        }
                        continue;
                    }

                    advance(); // {
                    advance(); // #
                    if (trimBefore) advance(); // -
                    skipComment();
                    continue;
                }

                // If expression or block tag
                const size_t tagPos = m_cursor;
                bool trimBefore = (m_cursor + 2 < m_source.size() && m_source[m_cursor + 2] == '-');

                if (tagPos > textStart) {
                    std::string_view chunk = m_source.substr(textStart, tagPos - textStart);
                    if (trimBefore) {
                        chunk = utils::trimTrailingWhitespace(chunk);
                    }
                    // Return raw text slice before entering tag
                    return JinjaToken{JinjaTokenType::Text, chunk, startLine, startCol};
                }

                // We are at the tag opening
                m_state = LexerState::InTag;
                if (next == '{') {
                    advance(); // {
                    advance(); // {
                    if (trimBefore) advance(); // -
                    return JinjaToken{JinjaTokenType::ExprBegin, m_source.substr(tagPos, m_cursor - tagPos), startLine, startCol};
                }
                if (next == '%') {
                    advance(); // {
                    advance(); // %
                    if (trimBefore) advance(); // -
                    return JinjaToken{JinjaTokenType::BlockBegin, m_source.substr(tagPos, m_cursor - tagPos), startLine, startCol};
                }
            }
        }
        advance();
    }

    if (m_cursor > textStart) {
        std::string_view chunk = m_source.substr(textStart, m_cursor - textStart);
        return JinjaToken{JinjaTokenType::Text, chunk, startLine, startCol};
    }

    return JinjaToken{JinjaTokenType::Eof, {}, m_line, m_column};
}

void JinjaLexer::skipWhitespaceInsideTag() noexcept
{
    while (!atEnd()) {
        const char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        } else {
            break;
        }
    }
}

JinjaToken JinjaLexer::scanTagToken()
{
    skipWhitespaceInsideTag();

    if (atEnd()) {
        return JinjaToken{JinjaTokenType::Eof, {}, m_line, m_column};
    }

    const size_t startLine = m_line;
    const size_t startCol  = m_column;
    const size_t startPos  = m_cursor;
    const char c = peek();

    // Tag Closings
    if (c == '-' && peekNext() == '}') {
        if (m_cursor + 2 < m_source.size() && m_source[m_cursor + 2] == '}') {
            advance(); // -
            advance(); // }
            advance(); // }
            m_state = LexerState::Text;
            m_stripLeadingWhitespace = true;
            return JinjaToken{JinjaTokenType::ExprEnd, m_source.substr(startPos, 3), startLine, startCol};
        }
    }
    if (c == '-' && peekNext() == '%') {
        if (m_cursor + 2 < m_source.size() && m_source[m_cursor + 2] == '}') {
            advance(); // -
            advance(); // %
            advance(); // }
            m_state = LexerState::Text;
            m_stripLeadingWhitespace = true;
            return JinjaToken{JinjaTokenType::BlockEnd, m_source.substr(startPos, 3), startLine, startCol};
        }
    }
    if (c == '}' && peekNext() == '}') {
        advance(); // }
        advance(); // }
        m_state = LexerState::Text;
        m_stripLeadingWhitespace = false;
        return JinjaToken{JinjaTokenType::ExprEnd, m_source.substr(startPos, 2), startLine, startCol};
    }
    if (c == '%' && peekNext() == '}') {
        advance(); // %
        advance(); // }
        m_state = LexerState::Text;
        m_stripLeadingWhitespace = false;
        return JinjaToken{JinjaTokenType::BlockEnd, m_source.substr(startPos, 2), startLine, startCol};
    }

    // String Literals ('...' or "...")
    if (c == '"' || c == '\'')
        return scanString(c);

    // Number Literals
    if (std::isdigit(static_cast<unsigned char>(c)))
        return scanNumber();

    // Identifiers & Keywords
    if (utils::isIdentStart(c))
        return scanIdentifierOrKeyword();

    // Operators & Punctuation
    advance(); // Consume operator character

    if (c == '=' && peek() == '=') {
        advance();
        return JinjaToken{JinjaTokenType::Eq, m_source.substr(startPos, 2), startLine, startCol};
    }
    if (c == '!' && peek() == '=') {
        advance();
        return JinjaToken{JinjaTokenType::Ne, m_source.substr(startPos, 2), startLine, startCol};
    }
    if (c == '<' && peek() == '=') {
        advance();
        return JinjaToken{JinjaTokenType::Le, m_source.substr(startPos, 2), startLine, startCol};
    }
    if (c == '>' && peek() == '=') {
        advance();
        return JinjaToken{JinjaTokenType::Ge, m_source.substr(startPos, 2), startLine, startCol};
    }

    switch (c) {
    case '.': return JinjaToken{JinjaTokenType::Dot, m_source.substr(startPos, 1), startLine, startCol};
    case ',': return JinjaToken{JinjaTokenType::Comma, m_source.substr(startPos, 1), startLine, startCol};
    case ':': return JinjaToken{JinjaTokenType::Colon, m_source.substr(startPos, 1), startLine, startCol};
    case '|': return JinjaToken{JinjaTokenType::Pipe, m_source.substr(startPos, 1), startLine, startCol};
    case '~': return JinjaToken{JinjaTokenType::Tilde, m_source.substr(startPos, 1), startLine, startCol};
    case '+': return JinjaToken{JinjaTokenType::Plus, m_source.substr(startPos, 1), startLine, startCol};
    case '-': return JinjaToken{JinjaTokenType::Minus, m_source.substr(startPos, 1), startLine, startCol};
    case '*': return JinjaToken{JinjaTokenType::Mul, m_source.substr(startPos, 1), startLine, startCol};
    case '/': return JinjaToken{JinjaTokenType::Div, m_source.substr(startPos, 1), startLine, startCol};
    case '%': return JinjaToken{JinjaTokenType::Mod, m_source.substr(startPos, 1), startLine, startCol};
    case '<': return JinjaToken{JinjaTokenType::Lt, m_source.substr(startPos, 1), startLine, startCol};
    case '>': return JinjaToken{JinjaTokenType::Gt, m_source.substr(startPos, 1), startLine, startCol};
    case '=': return JinjaToken{JinjaTokenType::Assign, m_source.substr(startPos, 1), startLine, startCol};
    case '(': return JinjaToken{JinjaTokenType::ParenOpen, m_source.substr(startPos, 1), startLine, startCol};
    case ')': return JinjaToken{JinjaTokenType::ParenClose, m_source.substr(startPos, 1), startLine, startCol};
    case '[': return JinjaToken{JinjaTokenType::BracketOpen, m_source.substr(startPos, 1), startLine, startCol};
    case ']': return JinjaToken{JinjaTokenType::BracketClose, m_source.substr(startPos, 1), startLine, startCol};
    case '{': return JinjaToken{JinjaTokenType::BraceOpen, m_source.substr(startPos, 1), startLine, startCol};
    case '}': return JinjaToken{JinjaTokenType::BraceClose, m_source.substr(startPos, 1), startLine, startCol};
    default:  return JinjaToken{JinjaTokenType::Unknown, m_source.substr(startPos, 1), startLine, startCol};
    }
}

JinjaToken JinjaLexer::scanString(char quote)
{
    const size_t startLine = m_line;
    const size_t startCol  = m_column;
    advance(); // Consume opening quote
    const size_t contentStart = m_cursor;

    while (!atEnd()) {
        const char c = peek();
        if (c == '\\') {
            advance(); // Skip escape backslash
            if (!atEnd()) advance(); // Skip escaped character
            continue;
        }
        if (c == quote) {
            const size_t contentEnd = m_cursor;
            advance(); // Consume closing quote
            return JinjaToken{JinjaTokenType::StringLiteral, m_source.substr(contentStart, contentEnd - contentStart), startLine, startCol};
        }
        advance();
    }

    //  Why didn't you end
    return JinjaToken{JinjaTokenType::StringLiteral, m_source.substr(contentStart), startLine, startCol};
}

JinjaToken JinjaLexer::scanNumber()
{
    const size_t startLine = m_line;
    const size_t startCol  = m_column;
    const size_t startPos  = m_cursor;

    while (!atEnd() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.')) {
        advance();
    }

    return JinjaToken{JinjaTokenType::NumberLiteral, m_source.substr(startPos, m_cursor - startPos), startLine, startCol};
}

JinjaToken JinjaLexer::scanIdentifierOrKeyword()
{
    const size_t startLine = m_line;
    const size_t startCol  = m_column;
    const size_t startPos  = m_cursor;

    while (!atEnd() && utils::isIdentBody(peek())) {
        advance();
    }

    const std::string_view ident = m_source.substr(startPos, m_cursor - startPos);

    // Keyword dispatch
    if (ident == "for")     return JinjaToken{JinjaTokenType::KwFor, ident, startLine, startCol};
    if (ident == "in")      return JinjaToken{JinjaTokenType::KwIn, ident, startLine, startCol};
    if (ident == "endfor")  return JinjaToken{JinjaTokenType::KwEndFor, ident, startLine, startCol};
    if (ident == "if")      return JinjaToken{JinjaTokenType::KwIf, ident, startLine, startCol};
    if (ident == "elif")    return JinjaToken{JinjaTokenType::KwElif, ident, startLine, startCol};
    if (ident == "else")    return JinjaToken{JinjaTokenType::KwElse, ident, startLine, startCol};
    if (ident == "endif")   return JinjaToken{JinjaTokenType::KwEndIf, ident, startLine, startCol};
    if (ident == "set")     return JinjaToken{JinjaTokenType::KwSet, ident, startLine, startCol};
    if (ident == "and")     return JinjaToken{JinjaTokenType::KwAnd, ident, startLine, startCol};
    if (ident == "or")      return JinjaToken{JinjaTokenType::KwOr, ident, startLine, startCol};
    if (ident == "not")     return JinjaToken{JinjaTokenType::KwNot, ident, startLine, startCol};
    if (ident == "is")      return JinjaToken{JinjaTokenType::KwIs, ident, startLine, startCol};
    if (ident == "true"  || ident == "True")  return JinjaToken{JinjaTokenType::KwTrue, ident, startLine, startCol};
    if (ident == "false" || ident == "False") return JinjaToken{JinjaTokenType::KwFalse, ident, startLine, startCol};
    if (ident == "none"  || ident == "None" || ident == "null") return JinjaToken{JinjaTokenType::KwNone, ident, startLine, startCol};

    return JinjaToken{JinjaTokenType::Identifier, ident, startLine, startCol};
}

} // namespace job::token