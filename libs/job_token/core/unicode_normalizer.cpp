#include "core/unicode_normalizer.h"

#include <cctype>

namespace job::token {

bool UnicodeNormalizer::isValidUtf8(std::string_view text) noexcept
{
    std::size_t offset = 0;
    std::uint32_t codepoint = 0;

    while (offset < text.size()) {
        if (!nextCodepoint(text, offset, codepoint))
            return false;
    }

    return true;
}

bool UnicodeNormalizer::nextCodepoint(std::string_view text, std::size_t &offset, std::uint32_t &outCodepoint) noexcept
{
    if (offset >= text.size())
        return false;

    const auto *bytes =
        reinterpret_cast<const std::uint8_t *>(text.data());

    const std::size_t length = text.size();
    const std::uint8_t b0 = bytes[offset];

    // ASCII: U+0000..U+007F
    if (b0 <= 0x7F) {
        outCodepoint = b0;
        ++offset;
        return true;
    }

    // 2-byte sequence: U+0080..U+07FF
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (offset + 1 >= length)
            return false;

        const std::uint8_t b1 = bytes[offset + 1];

        if ((b1 & 0xC0) != 0x80)
            return false;

        outCodepoint =
            (static_cast<std::uint32_t>(b0 & 0x1F) << 6) |
            static_cast<std::uint32_t>(b1 & 0x3F);

        offset += 2;
        return true;
    }

    // 3-byte sequence: U+0800..U+FFFF
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (offset + 2 >= length)
            return false;

        const std::uint8_t b1 = bytes[offset + 1];
        const std::uint8_t b2 = bytes[offset + 2];

        if ((b1 & 0xC0) != 0x80 ||
            (b2 & 0xC0) != 0x80) {
            return false;
        }

        // Reject overlong encodings.
        if (b0 == 0xE0 && b1 < 0xA0)
            return false;

        // Reject UTF-16 surrogate range U+D800..U+DFFF.
        if (b0 == 0xED && b1 > 0x9F)
            return false;

        outCodepoint =
            (static_cast<std::uint32_t>(b0 & 0x0F) << 12) |
            (static_cast<std::uint32_t>(b1 & 0x3F) << 6) |
            static_cast<std::uint32_t>(b2 & 0x3F);

        offset += 3;
        return true;
    }

    // 4-byte sequence: U+10000..U+10FFFF
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (offset + 3 >= length)
            return false;

        const std::uint8_t b1 = bytes[offset + 1];
        const std::uint8_t b2 = bytes[offset + 2];
        const std::uint8_t b3 = bytes[offset + 3];

        if ((b1 & 0xC0) != 0x80 ||
            (b2 & 0xC0) != 0x80 ||
            (b3 & 0xC0) != 0x80) {
            return false;
        }

        // Reject overlong encodings.
        if (b0 == 0xF0 && b1 < 0x90)
            return false;

        // Reject values above U+10FFFF.
        if (b0 == 0xF4 && b1 > 0x8F)
            return false;

        outCodepoint =
            (static_cast<std::uint32_t>(b0 & 0x07) << 18) |
            (static_cast<std::uint32_t>(b1 & 0x3F) << 12) |
            (static_cast<std::uint32_t>(b2 & 0x3F) << 6) |
            static_cast<std::uint32_t>(b3 & 0x3F);

        offset += 4;
        return true;
    }

    return false;
}

std::size_t UnicodeNormalizer::encodeCodepoint(std::uint32_t codepoint, char *outBuffer) noexcept
{
    if (!outBuffer)
        return 0;

    // UTF-16 surrogate code points are not Unicode scalar values.
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
        return 0;

    auto *bytes =
        reinterpret_cast<std::uint8_t *>(outBuffer);

    if (codepoint <= 0x7F) {
        bytes[0] = static_cast<std::uint8_t>(codepoint);
        return 1;
    }

    if (codepoint <= 0x7FF) {
        bytes[0] = static_cast<std::uint8_t>(
            0xC0 | (codepoint >> 6));

        bytes[1] = static_cast<std::uint8_t>(
            0x80 | (codepoint & 0x3F));

        return 2;
    }

    if (codepoint <= 0xFFFF) {
        bytes[0] = static_cast<std::uint8_t>(
            0xE0 | (codepoint >> 12));

        bytes[1] = static_cast<std::uint8_t>(
            0x80 | ((codepoint >> 6) & 0x3F));

        bytes[2] = static_cast<std::uint8_t>(
            0x80 | (codepoint & 0x3F));

        return 3;
    }

    if (codepoint <= 0x10FFFF) {
        bytes[0] = static_cast<std::uint8_t>(
            0xF0 | (codepoint >> 18));

        bytes[1] = static_cast<std::uint8_t>(
            0x80 | ((codepoint >> 12) & 0x3F));

        bytes[2] = static_cast<std::uint8_t>(
            0x80 | ((codepoint >> 6) & 0x3F));

        bytes[3] = static_cast<std::uint8_t>(
            0x80 | (codepoint & 0x3F));

        return 4;
    }

    return 0;
}

std::string UnicodeNormalizer::escapeSpaces(
    std::string_view text,
    bool prependDummySpace)
{
    if (text.empty())
        return prependDummySpace ? std::string{kSentencePieceSpace} : std::string{};

    std::string result;
    result.reserve(text.size() * 2);

    if (prependDummySpace &&
        text.front() != ' ' &&
        !text.starts_with(kSentencePieceSpace)) {
        result.append(kSentencePieceSpace);
    }

    for (const char c : text) {
        if (c == ' ')
            result.append(kSentencePieceSpace);
        else
            result.push_back(c);
    }

    return result;
}

std::string UnicodeNormalizer::unescapeSpaces(std::string_view text, bool stripLeadingDummySpace)
{
    if (text.empty())
        return {};

    std::string result;
    result.reserve(text.size());

    std::size_t offset = 0;

    if (stripLeadingDummySpace &&
        text.starts_with(kSentencePieceSpace)) {
        offset += kSentencePieceSpace.size();
    }

    while (offset < text.size()) {
        const std::string_view remaining = text.substr(offset);

        if (remaining.starts_with(kSentencePieceSpace)) {
            result.push_back(' ');
            offset += kSentencePieceSpace.size();
        } else {
            result.push_back(text[offset]);
            ++offset;
        }
    }

    return result;
}

std::string_view UnicodeNormalizer::trimWhitespace(
    std::string_view text) noexcept
{
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

std::string UnicodeNormalizer::normalize(std::string_view text, const Options &options)
{
    if (text.empty())
        return options.addDummyPrefixSpace ? std::string{kSentencePieceSpace} : std::string{};

    std::string normalized;
    normalized.reserve(text.size() * 2);

    if (options.cleanExtraSpaces) {
        bool inWhitespace = false;

        for (const char c : text) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!inWhitespace)
                    normalized.push_back(' ');

                inWhitespace = true;
            } else {
                normalized.push_back(c);
                inWhitespace = false;
            }
        }
    } else {
        normalized.assign(text);
    }

    if (options.replaceSpacesWithMarker) {
        return escapeSpaces(
            normalized,
            options.addDummyPrefixSpace);
    }

    if (options.addDummyPrefixSpace &&
        !normalized.empty() &&
        normalized.front() != ' ') {
        normalized.insert(normalized.begin(), ' ');
    }

    return normalized;
}

} // namespace job::token