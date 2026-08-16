#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT UnicodeNormalizer {
public:
    // SentencePiece space marker U+2581 (" ") in UTF-8
    static constexpr std::string_view kSentencePieceSpace = "\xE2\x96\x81";
    [[nodiscard]] static bool isValidUtf8(std::string_view text) noexcept;
    [[nodiscard]] static bool nextCodepoint(std::string_view text, size_t& offset, uint32_t& outCodepoint) noexcept;


    static size_t encodeCodepoint(uint32_t codepoint, char* outBuffer) noexcept;
    [[nodiscard]] static std::string escapeSpaces(std::string_view text, bool prependDummySpace = true);

    [[nodiscard]] static std::string unescapeSpaces(std::string_view text, bool stripLeadingDummySpace = false);
    [[nodiscard]] static std::string_view trimWhitespace(std::string_view text) noexcept;

    struct Options {
        bool addDummyPrefixSpace    = false;
        bool replaceSpacesWithMarker = false;
        bool treatWhitespaceAsToken = false;
        bool cleanExtraSpaces       = false;
    };

    [[nodiscard]] static std::string normalize(std::string_view text, const Options& opts);
};

} // namespace job::token