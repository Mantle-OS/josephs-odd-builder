#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "job_json_grammar.h"

    namespace job::json {

    class JsonEscapeKernel
    {
    public:
        [[nodiscard]] static bool decode(std::string_view value, std::string &result)
        {
            std::string decoded;
            decoded.reserve(value.size());

            std::size_t offset = 0;

            while (offset < value.size()) {
                const char c = value[offset++];

                if (c != '\\') {
                    decoded.push_back(c);
                    continue;
                }

                if (offset >= value.size())
                    return false;

                switch (value[offset++]) {
                case '"':
                    decoded.push_back('"');
                    break;
                case '\\':
                    decoded.push_back('\\');
                    break;
                case '/':
                    decoded.push_back('/');
                    break;
                case 'b':
                    decoded.push_back('\b');
                    break;
                case 'f':
                    decoded.push_back('\f');
                    break;
                case 'n':
                    decoded.push_back('\n');
                    break;
                case 'r':
                    decoded.push_back('\r');
                    break;
                case 't':
                    decoded.push_back('\t');
                    break;
                case 'u': {
                    std::uint32_t codepoint = 0;

                    if (!decodeHex4(value, offset, codepoint))
                        return false;

                    if (isHighSurrogate(codepoint)) {
                        if (offset + 6 > value.size() ||
                            value[offset] != '\\' ||
                            value[offset + 1] != 'u') {
                            return false;
                        }

                        offset += 2;

                        std::uint32_t low = 0;

                        if (!decodeHex4(value, offset, low) || !isLowSurrogate(low))
                            return false;

                        codepoint =
                            0x10000u +
                            ((codepoint - 0xD800u) << 10u) +
                            (low - 0xDC00u);
                    } else if (isLowSurrogate(codepoint)) {
                        return false;
                    }

                    if (!appendUtf8(codepoint, decoded))
                        return false;

                    break;
                }
                default:
                    return false;
                }
            }

            result = std::move(decoded);
            return true;
        }

    private:
        [[nodiscard]] static constexpr bool isHighSurrogate(std::uint32_t value) noexcept
        {
            return value >= 0xD800u && value <= 0xDBFFu;
        }

        [[nodiscard]] static constexpr bool isLowSurrogate(std::uint32_t value) noexcept
        {
            return value >= 0xDC00u && value <= 0xDFFFu;
        }

        [[nodiscard]] static constexpr std::uint8_t hexValue(char c) noexcept
        {
            if (c >= '0' && c <= '9')
                return static_cast<std::uint8_t>(c - '0');

            if (c >= 'a' && c <= 'f')
                return static_cast<std::uint8_t>(10 + c - 'a');

            if (c >= 'A' && c <= 'F')
                return static_cast<std::uint8_t>(10 + c - 'A');

            return 0xFFu;
        }

        [[nodiscard]] static bool decodeHex4( std::string_view value, std::size_t &offset, std::uint32_t &result) noexcept
        {
            if (offset + 4 > value.size())
                return false;

            std::uint32_t decoded = 0;

            for (std::size_t i = 0; i < 4; ++i) {
                const char c = value[offset + i];

                if (!grammar::is_HEXDIG(static_cast<char32_t>(c)))
                    return false;

                decoded = (decoded << 4u) | hexValue(c);
            }

            offset += 4;
            result = decoded;
            return true;
        }

        [[nodiscard]] static bool appendUtf8(std::uint32_t codepoint, std::string &result)
        {
            if (codepoint <= 0x7Fu) {
                result.push_back(static_cast<char>(codepoint));
                return true;
            }

            if (codepoint <= 0x7FFu) {
                result.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
                result.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
                return true;
            }

            if (codepoint >= 0xD800u && codepoint <= 0xDFFFu)
                return false;

            if (codepoint <= 0xFFFFu) {
                result.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
                result.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
                result.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
                return true;
            }

            if (codepoint <= 0x10FFFFu) {
                result.push_back(static_cast<char>(0xF0u | (codepoint >> 18u)));
                result.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu)));
                result.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
                result.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
                return true;
            }

            return false;
        }
    };

} // namespace job::json
