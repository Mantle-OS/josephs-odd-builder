#include "core/unicode_normalizer.h"

#include <cctype>

namespace job::token {

bool UnicodeNormalizer::isValidUtf8(std::string_view text) noexcept
{
    size_t offset = 0;
    uint32_t codepoint = 0;

    while (offset < text.size()) {
        if (!nextCodepoint(text, offset, codepoint))
            return false;
    }

    return true;
}

bool UnicodeNormalizer::nextCodepoint(std::string_view text, size_t& offset, uint32_t& outCodepoint) noexcept
{
    if (offset >= text.size())
        return false;

    const auto* const bytes = reinterpret_cast<const uint8_t*>(text.data());
    const size_t len = text.size();
    const uint8_t b0 = bytes[offset];

    // 1-byte ASCII: 0x00..0x7F
    if (b0 <= 0x7F) {
        outCodepoint = b0;
        offset += 1;
        return true;
    }

    // 2-byte sequence: 0xC2..0xDF + 1 continuation byte
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (offset + 1 >= len)
            return false;

        const uint8_t b1 = bytes[offset + 1];
        if ((b1 & 0xC0) != 0x80)
            return false;

        outCodepoint = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        offset += 2;
        return true;
    }

    // 3-byte sequence: 0xE0..0xEF + 2 continuation bytes
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (offset + 2 >= len)
            return false;
        const uint8_t b1 = bytes[offset + 1];
        const uint8_t b2 = bytes[offset + 2];

        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80)
            return false;

        // Overlong and surrogate rejection (RFC 3629)
        if (b0 == 0xE0 && b1 < 0xA0)
            return false; // Overlong

        if (b0 == 0xED && b1 > 0x9F)
            return false; // UTF-16 surrogate halves (0xD800..0xDFFF)

        outCodepoint = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        offset += 3;
        return true;
    }

    // 4-byte sequence: 0xF0..0xF4 + 3 continuation bytes
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (offset + 3 >= len)
            return false;

        const uint8_t b1 = bytes[offset + 1];
        const uint8_t b2 = bytes[offset + 2];
        const uint8_t b3 = bytes[offset + 3];

        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
            return false;

        // Overlong and out-of-range rejection
        if (b0 == 0xF0 && b1 < 0x90)
            return false; // Overlong

        if (b0 == 0xF4 && b1 > 0x8F)
            return false; // Above U+10FFFF

        outCodepoint = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        offset += 4;
        return true;
    }

    return false;
}

size_t UnicodeNormalizer::encodeCodepoint(uint32_t codepoint, char* outBuffer) noexcept
{
    if (!outBuffer)
        return 0;

    auto* bytes = reinterpret_cast<uint8_t*>(outBuffer);

    if (codepoint <= 0x7F) {
        bytes[0] = static_cast<uint8_t>(codepoint);
        return 1;
    }

    if (codepoint <= 0x7FF) {
        bytes[0] = static_cast<uint8_t>(0xC0 | (codepoint >> 6));
        bytes[1] = static_cast<uint8_t>(0x80 | (codepoint & 0x3F));
        return 2;
    }

    if (codepoint <= 0xFFFF) {
        bytes[0] = static_cast<uint8_t>(0xE0 | (codepoint >> 12));
        bytes[1] = static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F));
        bytes[2] = static_cast<uint8_t>(0x80 | (codepoint & 0x3F));
        return 3;
    }

    if (codepoint <= 0x10FFFF) {
        bytes[0] = static_cast<uint8_t>(0xF0 | (codepoint >> 18));
        bytes[1] = static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F));
        bytes[2] = static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F));
        bytes[3] = static_cast<uint8_t>(0x80 | (codepoint & 0x3F));
        return 4;
    }

    return 0;
}

std::string UnicodeNormalizer::escapeSpaces(std::string_view text, bool prependDummySpace)
{
    if (text.empty())
        return prependDummySpace ? std::string(kSentencePieceSpace) : std::string{};

    std::string result;
    result.reserve(text.size() * 2);

    if (prependDummySpace && text.front() != ' ' && !text.starts_with(kSentencePieceSpace)) {
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

    size_t i = 0;
    if (stripLeadingDummySpace && text.starts_with(kSentencePieceSpace))
        i += kSentencePieceSpace.size();

    while (i < text.size()) {
        if (text.substr(i).starts_with(kSentencePieceSpace)) {
            result.push_back(' ');
            i += kSentencePieceSpace.size();
        } else {
            result.push_back(text[i]);
            ++i;
        }
    }

    return result;
}

std::string_view UnicodeNormalizer::trimWhitespace(std::string_view text) noexcept
{
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])))
        ++start;

    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])))
        --end;

    return text.substr(start, end - start);
}

std::string UnicodeNormalizer::normalize(std::string_view text, const Options& opts)
{
    if (text.empty())
        return opts.addDummyPrefixSpace ? std::string(kSentencePieceSpace) : std::string{};

    std::string intermediate;
    intermediate.reserve(text.size() * 2);

    // Optional space cleaning (collapse consecutive whitespace to single space)
    if (opts.cleanExtraSpaces) {
        bool inSpace = false;
        for (const char c : text) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!inSpace) {
                    intermediate.push_back(' ');
                    inSpace = true;
                }
            } else {
                intermediate.push_back(c);
                inSpace = false;
            }
        }
    } else {
        intermediate = std::string(text);
    }

    // SentencePiece whitespace escape
    if (opts.replaceSpacesWithMarker) {
        return escapeSpaces(intermediate, opts.addDummyPrefixSpace);
    }

    if (opts.addDummyPrefixSpace && !intermediate.empty() && intermediate.front() != ' ')
        return " " + intermediate;

    return intermediate;
}

} // namespace job::token