#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "job_yaml_decoded_scalar.h"
#include "job_yaml_scalar_style.h"

namespace job::yaml {

class YamlScalarDecoder
{
public:
    YamlScalarDecoder() = delete;
    ~YamlScalarDecoder() = delete;

    YamlScalarDecoder(const YamlScalarDecoder &) = delete;
    YamlScalarDecoder &operator=(const YamlScalarDecoder &) = delete;

    YamlScalarDecoder(YamlScalarDecoder &&) = delete;
    YamlScalarDecoder &operator=(YamlScalarDecoder &&) = delete;

    [[nodiscard]] static bool decode(std::string_view source, YamlScalarStyle style, YamlDecodedScalar &result)
    {
        result.clear();

        switch (style) {
        case YamlScalarStyle::Plain:
            return decodePlain(source, result);

        case YamlScalarStyle::SingleQuoted:
            return decodeSingleQuoted(source, result);

        case YamlScalarStyle::DoubleQuoted:
            return decodeDoubleQuoted(source, result);

        case YamlScalarStyle::None:
            return false;
        }

        return false;
    }

private:
    [[nodiscard]] static bool decodePlain(std::string_view source,
                                          YamlDecodedScalar &result) noexcept
    {
        result.setBorrowed(source);
        return true;
    }

    [[nodiscard]] static bool decodeSingleQuoted(std::string_view source, YamlDecodedScalar &result)
    {
        if (source.size() < 2 || source.front() != '\'' || source.back() != '\'')
            return false;

        const std::string_view content = source.substr(1, source.size() - 2);

        if (content.find('\'') == std::string_view::npos &&
            content.find('\n') == std::string_view::npos &&
            content.find('\r') == std::string_view::npos) {
            result.setBorrowed(content);
            return true;
        }

        return decodeTransformedSingleQuoted(content, result);
    }

    [[nodiscard]] static bool decodeDoubleQuoted(std::string_view source, YamlDecodedScalar &result)
    {
        if (source.size() < 2 || source.front() != '"' || source.back() != '"')
            return false;

        const std::string_view content = source.substr(1, source.size() - 2);

        if (content.find('\\') == std::string_view::npos &&
            content.find('\n') == std::string_view::npos &&
            content.find('\r') == std::string_view::npos) {
            result.setBorrowed(content);
            return true;
        }

        return decodeTransformedDoubleQuoted(content, result);
    }

    [[nodiscard]] static bool decodeTransformedSingleQuoted(std::string_view source, YamlDecodedScalar &result)
    {
        std::string storage;
        storage.reserve(source.size());

        for (std::size_t i = 0; i < source.size(); ++i) {
            if (source[i] != '\'') {
                storage.push_back(source[i]);
                continue;
            }

            if (i + 1 >= source.size() || source[i + 1] != '\'')
                return false;

            storage.push_back('\'');
            ++i;
        }

        result.setTransformed(std::move(storage));
        return true;
    }

    [[nodiscard]] static bool decodeTransformedDoubleQuoted(std::string_view source, YamlDecodedScalar &result)
    {
        std::string storage;
        storage.reserve(source.size());

        for (std::size_t i = 0; i < source.size(); ++i) {
            const char c = source[i];

            if (c == '\n' || c == '\r')
                return false;

            if (c != '\\') {
                storage.push_back(c);
                continue;
            }

            if (++i >= source.size())
                return false;

            switch (source[i]) {
            case '0':
                storage.push_back('\0');
                break;

            case 'a':
                storage.push_back('\a');
                break;

            case 'b':
                storage.push_back('\b');
                break;

            case 't':
            case '\t':
                storage.push_back('\t');
                break;

            case 'n':
                storage.push_back('\n');
                break;

            case 'v':
                storage.push_back('\v');
                break;

            case 'f':
                storage.push_back('\f');
                break;

            case 'r':
                storage.push_back('\r');
                break;

            case 'e':
                storage.push_back('\x1B');
                break;

            case ' ':
                storage.push_back(' ');
                break;

            case '"':
                storage.push_back('"');
                break;

            case '/':
                storage.push_back('/');
                break;

            case '\\':
                storage.push_back('\\');
                break;

            case 'N':
                if (!appendUtf8(0x0085, storage))
                    return false;
                break;

            case '_':
                if (!appendUtf8(0x00A0, storage))
                    return false;
                break;

            case 'L':
                if (!appendUtf8(0x2028, storage))
                    return false;
                break;

            case 'P':
                if (!appendUtf8(0x2029, storage))
                    return false;
                break;

            case 'x': {
                char32_t codepoint{};

                if (!decodeHexCodepoint(source, i + 1, 2, codepoint))
                    return false;

                if (!appendUtf8(codepoint, storage))
                    return false;

                i += 2;
                break;
            }

            case 'u': {
                char32_t codepoint{};

                if (!decodeHexCodepoint(source, i + 1, 4, codepoint))
                    return false;

                if (!appendUtf8(codepoint, storage))
                    return false;

                i += 4;
                break;
            }

            case 'U': {
                char32_t codepoint{};

                if (!decodeHexCodepoint(source, i + 1, 8, codepoint))
                    return false;

                if (!appendUtf8(codepoint, storage))
                    return false;

                i += 8;
                break;
            }

            default:
                return false;
            }
        }

        result.setTransformed(std::move(storage));
        return true;
    }

    [[nodiscard]] static constexpr int hexValue(char value) noexcept
    {
        if (value >= '0' && value <= '9')
            return value - '0';

        if (value >= 'a' && value <= 'f')
            return value - 'a' + 10;

        if (value >= 'A' && value <= 'F')
            return value - 'A' + 10;

        return -1;
    }

    [[nodiscard]] static bool decodeHexCodepoint(std::string_view source,
                                                 std::size_t offset,
                                                 std::size_t digits,
                                                 char32_t &codepoint) noexcept
    {
        if (offset > source.size() || digits > source.size() - offset)
            return false;

        char32_t value{};

        for (std::size_t i = 0; i < digits; ++i) {
            const int digit = hexValue(source[offset + i]);

            if (digit < 0)
                return false;

            value = static_cast<char32_t>((value << 4) |
                                          static_cast<char32_t>(digit));
        }

        codepoint = value;
        return true;
    }

    [[nodiscard]] static bool appendUtf8(char32_t codepoint, std::string &storage)
    {
        if (codepoint <= 0x7F) {
            storage.push_back(static_cast<char>(codepoint));
            return true;
        }

        if (codepoint <= 0x7FF) {
            storage.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            storage.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            return true;
        }

        if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
            return false;

        if (codepoint <= 0xFFFF) {
            storage.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            storage.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            storage.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            return true;
        }

        if (codepoint <= 0x10FFFF) {
            storage.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            storage.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            storage.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            storage.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            return true;
        }

        return false;
    }
};

} // namespace job::yaml