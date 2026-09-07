#pragma once

#include <cstddef>
#include <string_view>

#include "job_yaml_cursor.h"
#include "job_yaml_grammar.h"
#include "job_yaml_lex_type.h"

namespace job::yaml {

class YamlLex
{
public:
    explicit constexpr YamlLex(std::string_view source) noexcept :
        m_cursor(source)
    {
    }

    explicit constexpr YamlLex(YamlCursor cursor) noexcept :
        m_cursor(cursor)
    {
    }

    constexpr YamlLexeme next() noexcept
    {
        const bool lineStart = m_lineStart;

        if (m_lineStart) {
            if (m_cursor.current() == Space || m_cursor.current() == Tab)
                return scanIndent();

            m_lineStart = false;
        } else {
            skipInlineWhitespace();
        }

        if (!m_cursor)
            return {
                .type = YamlLexType::End,
                .scalarStyle = YamlScalarStyle::None,
                .text = {},
                .offset = m_cursor.offset()
            };

        if (lineStart) {
            if (isDocumentMarker(DirectivesEnd))
                return scanDocumentMarker(YamlLexType::DocumentStart);

            if (isDocumentMarker(DocumentEnd))
                return scanDocumentMarker(YamlLexType::DocumentEnd);

            if (m_cursor.current() == Directive) {
                const std::size_t start = m_cursor.offset();
                m_cursor.advance();
                return lexeme(YamlLexType::Directive, start);
            }
        }

        const std::size_t start = m_cursor.offset();
        const char c = m_cursor.current();

        switch (c) {
        case SequenceEntry:
            if (!isBlockIndicatorTerminator(m_cursor.peek()))
                return scanScalar();

            m_cursor.advance();
            return lexeme(YamlLexType::SequenceEntry, start);

        case MappingKey:
            m_cursor.advance();
            return lexeme(YamlLexType::MappingKey, start);

        case MappingValue:
            m_cursor.advance();
            return lexeme(YamlLexType::MappingValue, start);

        case SequenceStart:
            m_cursor.advance();
            ++m_flowDepth;
            return lexeme(YamlLexType::FlowSequenceStart, start);

        case SequenceEnd:
            m_cursor.advance();

            if (m_flowDepth > 0)
                --m_flowDepth;

            return lexeme(YamlLexType::FlowSequenceEnd, start);

        case MappingStart:
            m_cursor.advance();
            ++m_flowDepth;
            return lexeme(YamlLexType::FlowMappingStart, start);

        case MappingEnd:
            m_cursor.advance();

            if (m_flowDepth > 0)
                --m_flowDepth;

            return lexeme(YamlLexType::FlowMappingEnd, start);

        case CollectEntry:
            m_cursor.advance();
            return lexeme(YamlLexType::CollectEntry, start);

        case Anchor:
            return scanNodeProperty(YamlLexType::Anchor);

        case Alias:
            return scanNodeProperty(YamlLexType::Alias);

        case Tag:
            return scanTag();

        case Literal:
            return scanBlockScalarHeader(YamlLexType::Literal);

        case Folded:
            return scanBlockScalarHeader(YamlLexType::Folded);

        case SingleQuote:
            return scanQuoted(YamlScalarStyle::SingleQuoted, SingleQuote);

        case DoubleQuote:
            return scanQuoted(YamlScalarStyle::DoubleQuoted, DoubleQuote);

        case Comment:
            return scanComment();

        case '\n':
            m_cursor.advance();
            m_lineStart = true;
            return lexeme(YamlLexType::LineBreak, start);

        case '\r':
            m_cursor.advance();

            if (m_cursor.current() == '\n')
                m_cursor.advance();

            m_lineStart = true;
            return lexeme(YamlLexType::LineBreak, start);

        default:
            return scanScalar();
        }
    }

    [[nodiscard]] constexpr std::string_view source() const noexcept
    {
        return m_cursor.source();
    }

    [[nodiscard]] constexpr const YamlCursor &cursor() const noexcept
    {
        return m_cursor;
    }

    [[nodiscard]] constexpr YamlCursor &cursor() noexcept
    {
        return m_cursor;
    }

    constexpr void resetAtLineStart(std::size_t offset) noexcept
    {
        m_cursor.reset(offset);
        m_flowDepth = 0;
        m_lineStart = true;
    }

private:
    constexpr void skipInlineWhitespace() noexcept
    {
        while (m_cursor.current() == Space || m_cursor.current() == Tab)
            m_cursor.advance();
    }

    [[nodiscard]] static constexpr bool isNodePropertyTerminator(char c) noexcept
    {
        return c == '\0' ||
               c == Space ||
               c == Tab ||
               c == '\n' ||
               c == '\r' ||
               c == CollectEntry ||
               c == SequenceStart ||
               c == SequenceEnd ||
               c == MappingStart ||
               c == MappingEnd;
    }

    [[nodiscard]] static constexpr bool isBlockIndicatorTerminator(char c) noexcept
    {
        return c == '\0' ||
               c == Space ||
               c == Tab ||
               c == '\n' ||
               c == '\r';
    }

    [[nodiscard]] static constexpr bool isBlockIndentIndicator(char c) noexcept
    {
        return c >= '1' && c <= '9';
    }

    [[nodiscard]] static constexpr bool isBlockChompingIndicator(char c) noexcept
    {
        return c == '+' || c == '-';
    }

    [[nodiscard]] static constexpr bool isDocumentMarkerTerminator(char c) noexcept
    {
        return c == '\0' ||
               c == Space ||
               c == Tab ||
               c == '\n' ||
               c == '\r';
    }

    [[nodiscard]] constexpr bool isDocumentMarker(std::string_view marker) const noexcept
    {
        const std::string_view source = m_cursor.source();
        const std::size_t offset = m_cursor.offset();

        if (offset > source.size() || marker.size() > source.size() - offset)
            return false;

        if (source.substr(offset, marker.size()) != marker)
            return false;

        const std::size_t end = offset + marker.size();
        const char next = end < source.size() ? source[end] : '\0';

        return isDocumentMarkerTerminator(next);
    }

    [[nodiscard]] constexpr YamlLexeme scanDocumentMarker(YamlLexType type) noexcept
    {
        const std::size_t start = m_cursor.offset();
        m_cursor.advance(3);
        return lexeme(type, start);
    }

    [[nodiscard]] constexpr YamlLexeme scanIndent() noexcept
    {
        const std::size_t start = m_cursor.offset();

        while (m_cursor.current() == Space || m_cursor.current() == Tab)
            m_cursor.advance();

        m_lineStart = false;
        return lexeme(YamlLexType::Indent, start);
    }

    [[nodiscard]] constexpr YamlLexeme lexeme(YamlLexType type,
                                              std::size_t start) const noexcept
    {
        return lexeme(type, {
                                .offset = start,
                                .size = m_cursor.offset() - start
                            });
    }

    [[nodiscard]] constexpr YamlLexeme lexeme(YamlLexType type,
                                              std::size_t start,
                                              std::size_t end) const noexcept
    {
        return lexeme(type, {
                                .offset = start,
                                .size = end - start
                            });
    }

    [[nodiscard]] constexpr YamlLexeme lexeme(YamlLexType type,
                                              YamlSourceRange range) const noexcept
    {
        return {
            .type = type,
            .scalarStyle = YamlScalarStyle::None,
            .text = range.view(m_cursor.source()),
            .offset = range.offset
        };
    }

    [[nodiscard]] constexpr YamlLexeme lexeme(YamlLexType type,
                                              YamlScalarStyle style,
                                              std::size_t start) const noexcept
    {
        return lexeme(type, style, {
                                       .offset = start,
                                       .size = m_cursor.offset() - start
                                   });
    }

    [[nodiscard]] constexpr YamlLexeme lexeme(YamlLexType type,
                                              YamlScalarStyle style,
                                              std::size_t start,
                                              std::size_t end) const noexcept
    {
        return lexeme(type, style, {
                                       .offset = start,
                                       .size = end - start
                                   });
    }

    [[nodiscard]] constexpr YamlLexeme lexeme(YamlLexType type,
                                              YamlScalarStyle style,
                                              YamlSourceRange range) const noexcept
    {
        return {
            .type = type,
            .scalarStyle = style,
            .text = range.view(m_cursor.source()),
            .offset = range.offset
        };
    }

    [[nodiscard]] constexpr YamlLexeme scanNodeProperty(YamlLexType type) noexcept
    {
        const std::size_t start = m_cursor.offset();

        m_cursor.advance();

        while (m_cursor && !isNodePropertyTerminator(m_cursor.current()))
            m_cursor.advance();

        return lexeme(type, start);
    }

    [[nodiscard]] constexpr YamlLexeme scanTag() noexcept
    {
        const std::size_t start = m_cursor.offset();

        m_cursor.advance();

        if (m_cursor.current() == '<') {
            m_cursor.advance();

            while (m_cursor && m_cursor.current() != '>')
                m_cursor.advance();

            if (m_cursor.current() == '>')
                m_cursor.advance();

            return lexeme(YamlLexType::Tag, start);
        }

        while (m_cursor && !isNodePropertyTerminator(m_cursor.current()))
            m_cursor.advance();

        return lexeme(YamlLexType::Tag, start);
    }

    [[nodiscard]] constexpr YamlLexeme scanBlockScalarHeader(YamlLexType type) noexcept
    {
        const std::size_t start = m_cursor.offset();

        m_cursor.advance();

        bool hasIndentIndicator = false;
        bool hasChompingIndicator = false;

        for (std::size_t index = 0; index < 2 && m_cursor; ++index) {
            const char c = m_cursor.current();

            if (!hasIndentIndicator && isBlockIndentIndicator(c)) {
                hasIndentIndicator = true;
                m_cursor.advance();
                continue;
            }

            if (!hasChompingIndicator && isBlockChompingIndicator(c)) {
                hasChompingIndicator = true;
                m_cursor.advance();
                continue;
            }

            break;
        }

        return lexeme(type, start);
    }

    [[nodiscard]] constexpr YamlLexeme scanComment() noexcept
    {
        const std::size_t start = m_cursor.offset();

        while (m_cursor && m_cursor.current() != '\n' && m_cursor.current() != '\r')
            m_cursor.advance();

        return lexeme(YamlLexType::Comment, start);
    }

    [[nodiscard]] constexpr YamlLexeme scanQuoted(YamlScalarStyle style,
                                                  char quote) noexcept
    {
        const std::size_t start = m_cursor.offset();

        m_cursor.advance();

        while (m_cursor) {
            const char c = m_cursor.current();

            if (quote == SingleQuote &&
                c == SingleQuote &&
                m_cursor.peek() == SingleQuote) {
                m_cursor.advance(2);
                continue;
            }

            if (quote == DoubleQuote &&
                c == Escape &&
                m_cursor.peek() != '\0') {
                m_cursor.advance(2);
                continue;
            }

            if (c == quote) {
                m_cursor.advance();
                break;
            }

            m_cursor.advance();
        }

        return lexeme(YamlLexType::Scalar, style, start);
    }

    [[nodiscard]] constexpr YamlLexeme scanScalar() noexcept
    {
        const std::size_t start = m_cursor.offset();

        while (m_cursor) {
            const char c = m_cursor.current();

            if (c == '\n' || c == '\r')
                break;

            if (m_flowDepth > 0 &&
                (c == CollectEntry ||
                 c == SequenceStart ||
                 c == SequenceEnd ||
                 c == MappingStart ||
                 c == MappingEnd))
                break;

            if (c == Comment &&
                (m_cursor.offset() == start ||
                 m_cursor.source()[m_cursor.offset() - 1] == Space ||
                 m_cursor.source()[m_cursor.offset() - 1] == Tab))
                break;

            if ((c == MappingValue || c == SequenceEntry) &&
                isBlockIndicatorTerminator(m_cursor.peek()))
                break;

            m_cursor.advance();
        }

        std::size_t end = m_cursor.offset();

        while (end > start &&
               (m_cursor.source()[end - 1] == Space ||
                m_cursor.source()[end - 1] == Tab))
            --end;

        return lexeme(YamlLexType::Scalar,
                      YamlScalarStyle::Plain,
                      start,
                      end);
    }

    YamlCursor      m_cursor;
    std::size_t     m_flowDepth{};
    bool            m_lineStart{true};
};

} // namespace job::yaml