#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "job_yaml_grammar.h"

namespace job::yaml {

struct YamlEscapeResult
{
    char32_t codepoint{};
    std::uint8_t consumed{};
    bool valid{};

    constexpr explicit operator bool() const noexcept
    {
        return valid;
    }
};

class YamlEscapeKernel
{
public:
    static constexpr YamlEscapeResult decode(std::string_view source) noexcept
    {
        if (source.size() < 2 || source[0] != Escape)
            return {};

        switch (source[1]) {
        case EscNull:
            return single(U'\0');

        case EscBell:
            return single(U'\a');

        case EscBackspace:
            return single(U'\b');

        case EscLineFeed:
            return single(U'\n');

        case EscVerticalTab:
            return single(U'\v');

        case EscFormFeed:
            return single(U'\f');

        case EscCarriageReturn:
            return single(U'\r');

        case EscEscape:
            return single(U'\x1B');

        case 't':
        case '\t':
            return single(U'\t');

        case ' ':
            return single(U' ');

        case EscDoubleQuote:
            return single(U'"');

        case EscSlash:
            return single(U'/');

        case EscBackslash:
            return single(U'\\');

        case EscNextLine:
            return single(U'\u0085');

        case EscNonBreakingSpace:
            return single(U'\u00A0');

        case EscLineSeparator:
            return single(U'\u2028');

        case EscParagraphSeparator:
            return single(U'\u2029');

        case 'x':
            return decodeHex(source, 2);

        case 'u':
            return decodeHex(source, 4);

        case 'U':
            return decodeHex(source, 8);

        default:
            return {};
        }
    }

    static constexpr bool isUnicodeScalar(char32_t codepoint) noexcept
    {
        return codepoint <= 0x10FFFF &&
               !(codepoint >= 0xD800 && codepoint <= 0xDFFF);
    }

    static constexpr std::uint8_t utf8Size(char32_t codepoint) noexcept
    {
        if (!isUnicodeScalar(codepoint))
            return 0;

        if (codepoint <= 0x7F)
            return 1;

        if (codepoint <= 0x7FF)
            return 2;

        if (codepoint <= 0xFFFF)
            return 3;

        return 4;
    }

    static constexpr std::uint8_t encodeUtf8(char32_t codepoint, char *output) noexcept
    {
        if (!isUnicodeScalar(codepoint))
            return 0;

        if (codepoint <= 0x7F) {
            output[0] = static_cast<char>(codepoint);
            return 1;
        }

        if (codepoint <= 0x7FF) {
            output[0] = static_cast<char>(0xC0 | (codepoint >> 6));
            output[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
            return 2;
        }

        if (codepoint <= 0xFFFF) {
            output[0] = static_cast<char>(0xE0 | (codepoint >> 12));
            output[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            output[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
            return 3;
        }

        output[0] = static_cast<char>(0xF0 | (codepoint >> 18));
        output[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        output[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        output[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
        return 4;
    }

private:
    static constexpr YamlEscapeResult single(char32_t codepoint) noexcept
    {
        return {
            .codepoint = codepoint,
            .consumed = 2,
            .valid = true
        };
    }

    static constexpr int hexValue(char c) noexcept
    {
        if (c >= '0' && c <= '9')
            return c - '0';

        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;

        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;

        return -1;
    }

    static constexpr YamlEscapeResult decodeHex(std::string_view source, std::uint8_t digits) noexcept
    {
        const std::size_t required = static_cast<std::size_t>(digits) + 2;

        if (source.size() < required)
            return {};

        char32_t codepoint = 0;

        for (std::size_t i = 0; i < digits; ++i) {
            const int digit = hexValue(source[i + 2]);

            if (digit < 0)
                return {};

            codepoint = static_cast<char32_t>((codepoint << 4) | static_cast<char32_t>(digit));
        }

        if (!isUnicodeScalar(codepoint))
            return {};

        return {
            .codepoint = codepoint,
            .consumed = static_cast<std::uint8_t>(required),
            .valid = true
        };
    }
};

} // namespace job::yaml