#pragma once

#include <cstdarg>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "job_ansi_enums.h"

namespace job::ansi::utils {

using namespace job::ansi::utils::enums;

class CharsetTranslator
{
public:
    using CharsetMap = std::unordered_map<char32_t, char32_t>;

    using Ptr = std::shared_ptr<CharsetTranslator>;
    using WPtr = std::weak_ptr<CharsetTranslator>;
    using UPtr = std::unique_ptr<CharsetTranslator>;

    explicit CharsetTranslator() noexcept;
    ~CharsetTranslator() = default;

    CharsetTranslator(const CharsetTranslator &) = default;
    CharsetTranslator &operator=(const CharsetTranslator &) = default;
    CharsetTranslator(CharsetTranslator &&) noexcept = default;
    CharsetTranslator &operator=(CharsetTranslator &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<CharsetTranslator>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<CharsetTranslator>();
    }

    enum class CharsetSet : std::uint8_t
    {
        G0 = 0,
        G1,
        G2,
        G3
    };

    // Select character set for a given Gx
    constexpr void selectCharset(CharsetSet set, CharsetDesignator designator) noexcept;
    constexpr void setCharSet(CharsetSet set) noexcept;

    [[nodiscard]] constexpr CharsetSet charSet() const noexcept;
    [[nodiscard]] constexpr CharsetDesignator activeCharset() const noexcept;

    // Charset designators for each Gx bank
    [[nodiscard]] constexpr CharsetDesignator g0() const noexcept;
    [[nodiscard]] constexpr CharsetDesignator g1() const noexcept;
    [[nodiscard]] constexpr CharsetDesignator g2() const noexcept;
    [[nodiscard]] constexpr CharsetDesignator g3() const noexcept;

    // Translate a character using the currently active charset
    [[nodiscard]] char32_t applyCharset(char32_t ch) const noexcept;
    [[nodiscard]] char32_t resolveCharsetChar(char32_t ch) const noexcept;

    // Expose charset translation map for a given designator
    [[nodiscard]] const CharsetMap &getMap(CharsetDesignator designator) const noexcept;

    // Expose a helper to map a char for a specific charset
    [[nodiscard]] char32_t mapCharForCharset(char32_t ch, CharsetDesignator charset) const noexcept;

private:
    // Charset map fetcher for a specific Gx
    [[nodiscard]] constexpr CharsetDesignator getDesignatorFor(CharsetSet set) const noexcept;

    CharsetSet m_charSet = CharsetSet::G0;
    CharsetDesignator m_g0 = CharsetDesignator::US_ASCII;
    CharsetDesignator m_g1 = CharsetDesignator::US_ASCII;
    CharsetDesignator m_g2 = CharsetDesignator::US_ASCII;
    CharsetDesignator m_g3 = CharsetDesignator::US_ASCII;

    // Charset translation maps
    static const CharsetMap DEC_SPECIAL_GRAPHICS;
    static const CharsetMap DEC_SUPPLEMENTAL;
    static const CharsetMap DEC_TECHNICAL;
};

} // namespace job::ansi::utils