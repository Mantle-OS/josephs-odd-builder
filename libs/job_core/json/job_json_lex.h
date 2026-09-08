#pragma once

#include <cstddef>
#include <string_view>

#include "job_json_cursor.h"
#include "job_json_grammar.h"
#include "job_json_lex_type.h"
#include "job_json_source_range.h"

namespace job::json {

class JsonLex
{
public:
    constexpr JsonLex() noexcept = default;

    constexpr JsonLex(JsonLexType type, JsonSourceRange range, bool hasEscapes = false) noexcept :
        m_type(type),
        m_range(range),
        m_hasEscapes(hasEscapes)
    {
    }

    constexpr ~JsonLex() = default;

    constexpr JsonLex(const JsonLex &) noexcept = default;
    constexpr JsonLex &operator=(const JsonLex &) noexcept = default;
    constexpr JsonLex(JsonLex &&) noexcept = default;
    constexpr JsonLex &operator=(JsonLex &&) noexcept = default;

    [[nodiscard]] constexpr JsonLexType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr const JsonSourceRange &range() const noexcept
    {
        return m_range;
    }

    [[nodiscard]] constexpr bool hasEscapes() const noexcept
    {
        return m_hasEscapes;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_type != JsonLexType::Invalid;
    }

    [[nodiscard]] constexpr bool isEnd() const noexcept
    {
        return m_type == JsonLexType::End;
    }

    [[nodiscard]] constexpr std::string_view view(std::string_view source) const noexcept pre(m_range.validFor(source))
    {
        return m_range.view(source);
    }

private:
    JsonLexType     m_type{JsonLexType::Invalid};
    JsonSourceRange m_range{};
    bool            m_hasEscapes{};
};


class JsonLexer
{
public:
    constexpr explicit JsonLexer(std::string_view source) noexcept :
        m_cursor(source)
    {
    }

    constexpr ~JsonLexer() = default;

    JsonLexer(const JsonLexer &) = delete;
    JsonLexer &operator=(const JsonLexer &) = delete;
    constexpr JsonLexer(JsonLexer &&) noexcept = default;
    constexpr JsonLexer &operator=(JsonLexer &&) noexcept = default;

    [[nodiscard]] constexpr std::string_view source() const noexcept
    {
        return m_cursor.source();
    }

    [[nodiscard]] constexpr std::size_t offset() const noexcept
    {
        return m_cursor.offset();
    }

    [[nodiscard]] constexpr JsonLex next() noexcept
    {
        skipWhitespace();

        if (m_cursor.atEnd())
            return JsonLex{JsonLexType::End, JsonSourceRange{m_cursor.offset(), m_cursor.offset()}};

        const std::size_t begin = m_cursor.offset();

        switch (m_cursor.current()) {
        case '{':
            return single(JsonLexType::BeginObject);
        case '}':
            return single(JsonLexType::EndObject);
        case '[':
            return single(JsonLexType::BeginArray);
        case ']':
            return single(JsonLexType::EndArray);
        case ':':
            return single(JsonLexType::NameSeparator);
        case ',':
            return single(JsonLexType::ValueSeparator);
        case '"':
            return scanString();
        case 't':
            return scanLiteral(JsonLexType::True, grammar::true_);
        case 'f':
            return scanLiteral(JsonLexType::False, grammar::false_);
        case 'n':
            return scanLiteral(JsonLexType::Null, grammar::null);
        case '-':
            return scanNumber();
        default:
            break;
        }

        if (grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current())))
            return scanNumber();

        m_cursor.advance();
        return JsonLex{JsonLexType::Invalid, JsonSourceRange{begin, m_cursor.offset()}};
    }

private:
    [[nodiscard]] constexpr JsonLex single(JsonLexType type) noexcept
    {
        const std::size_t begin = m_cursor.offset();
        m_cursor.advance();
        return JsonLex{type, JsonSourceRange{begin, m_cursor.offset()}};
    }

    constexpr void skipWhitespace() noexcept
    {
        while (!m_cursor.atEnd()) {
            switch (m_cursor.current()) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                m_cursor.advance();
                break;
            default:
                return;
            }
        }
    }
    [[nodiscard]] constexpr bool isValueTerminator() const noexcept
    {
        if (m_cursor.atEnd())
            return true;

        switch (m_cursor.current()) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case ':':
        case ',':
        case ']':
        case '}':
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] constexpr JsonLex scanLiteral(JsonLexType type, std::string_view literal) noexcept
    {
        const std::size_t begin = m_cursor.offset();

        for (const char expected : literal) {
            if (m_cursor.atEnd() || m_cursor.current() != expected)
                return invalidFrom(begin);

            m_cursor.advance();
        }

        if (!isValueTerminator())
            return invalidFrom(begin);

        return JsonLex{type, JsonSourceRange{begin, m_cursor.offset()}};
    }

    [[nodiscard]] constexpr JsonLex scanString() noexcept
    {
        const std::size_t begin = m_cursor.offset();
        bool hasEscapes = false;

        m_cursor.advance();
        while (!m_cursor.atEnd()) {
            const unsigned char c = static_cast<unsigned char>(m_cursor.current());

            if (c == static_cast<unsigned char>('"')) {
                m_cursor.advance();
                return JsonLex{
                    JsonLexType::String,
                    JsonSourceRange{begin, m_cursor.offset()},
                    hasEscapes
                };
            }

            if (c == static_cast<unsigned char>('\\')) {
                hasEscapes = true;
                m_cursor.advance();

                if (m_cursor.atEnd())
                    return invalidFrom(begin);

                switch (m_cursor.current()) {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    m_cursor.advance();
                    continue;

                case 'u':
                    m_cursor.advance();

                    for (std::size_t i = 0; i < 4; ++i) {
                        if (m_cursor.atEnd() ||
                            !grammar::is_HEXDIG(static_cast<char32_t>(m_cursor.current()))) {
                            return invalidFrom(begin);
                        }

                        m_cursor.advance();
                    }

                    continue;

                default:
                    return invalidFrom(begin);
                }
            }

            if (c < 0x20)
                return invalidFrom(begin);

            m_cursor.advance();
        }

        return JsonLex{JsonLexType::Invalid, JsonSourceRange{begin, m_cursor.offset()}, hasEscapes};
    }

    [[nodiscard]] constexpr JsonLex scanNumber() noexcept
    {
        const std::size_t begin = m_cursor.offset();

        if (m_cursor.consume('-')) {
            if (m_cursor.atEnd())
                return invalidFrom(begin);
        }

        if (m_cursor.consume('0')) {
            if (!m_cursor.atEnd() && grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current())))
                return invalidFrom(begin);
        } else {
            if (m_cursor.atEnd() || !grammar::is_digit1_9(static_cast<char32_t>(m_cursor.current())))
                return invalidFrom(begin);
            m_cursor.advance();
            while (!m_cursor.atEnd() && grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current()))) {
                m_cursor.advance();
            }
        }

        if (!m_cursor.atEnd() && m_cursor.current() == grammar::decimal_point) {
            m_cursor.advance();
            if (m_cursor.atEnd() || !grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current())))
                return invalidFrom(begin);

            do {
                m_cursor.advance();
            } while (!m_cursor.atEnd() && grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current())));
        }

        if (!m_cursor.atEnd() && grammar::is_e(static_cast<char32_t>(m_cursor.current()))) {
            m_cursor.advance();

            if (!m_cursor.atEnd() && (m_cursor.current() == grammar::plus || m_cursor.current() == grammar::minus))
                m_cursor.advance();

            if (m_cursor.atEnd() || !grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current())))
                return invalidFrom(begin);

            do {
                m_cursor.advance();
            } while (!m_cursor.atEnd() && grammar::is_DIGIT(static_cast<char32_t>(m_cursor.current())));
        }

        if (!isValueTerminator())
            return invalidFrom(begin);

        return JsonLex{JsonLexType::Number, JsonSourceRange{begin, m_cursor.offset()}};
    }

    [[nodiscard]] constexpr JsonLex invalidFrom(std::size_t begin) noexcept
    {
        if (!m_cursor.atEnd())
            m_cursor.advance();

        return JsonLex{JsonLexType::Invalid, JsonSourceRange{begin, m_cursor.offset()}};
    }

    JsonCursor m_cursor;
};

} // namespace job::json

