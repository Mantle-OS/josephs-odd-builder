#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT UnicodeNormalizer
{
public:
    using Ptr  = std::shared_ptr<UnicodeNormalizer>;
    using WPtr = std::weak_ptr<UnicodeNormalizer>;
    using UPtr = std::unique_ptr<UnicodeNormalizer>;

    struct Options
    {
        bool addDummyPrefixSpace{false};
        bool replaceSpacesWithMarker{false};
        bool cleanExtraSpaces{false};
    };

    UnicodeNormalizer() = default;
    ~UnicodeNormalizer() = default;

    UnicodeNormalizer(const UnicodeNormalizer &) = default;
    UnicodeNormalizer &operator=(const UnicodeNormalizer &) = default;
    UnicodeNormalizer(UnicodeNormalizer &&) noexcept = default;
    UnicodeNormalizer &operator=(UnicodeNormalizer &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<UnicodeNormalizer>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<UnicodeNormalizer>();
    }

    // SentencePiece space marker U+2581 "▁" in UTF-8.
    static constexpr std::string_view kSentencePieceSpace = "\xE2\x96\x81";
    [[nodiscard]] static bool isValidUtf8(std::string_view text) noexcept;
    [[nodiscard]] static bool nextCodepoint(std::string_view text, std::size_t &offset, std::uint32_t &outCodepoint) noexcept;
    [[nodiscard]] static std::size_t encodeCodepoint(std::uint32_t codepoint, char *outBuffer) noexcept;
    [[nodiscard]] static std::string escapeSpaces(std::string_view text, bool prependDummySpace = true);
    [[nodiscard]] static std::string unescapeSpaces(std::string_view text, bool stripLeadingDummySpace = false);
    [[nodiscard]] static std::string_view trimWhitespace(std::string_view text) noexcept;
    [[nodiscard]] static std::string normalize(std::string_view text, const Options &options);
};

} // namespace job::token