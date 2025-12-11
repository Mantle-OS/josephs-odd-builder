#pragma once

#include <unordered_map>

#include "job_ansi_enums.h"

namespace job::ansi::utils {
using namespace job::ansi::utils::enums;

class CharsetTranslator {
public:
    CharsetTranslator();
    enum class CharsetSet {
        G0 = 0,
        G1 = 1,
        G2 = 2,
        G3 = 3
    };

    // Select character set for a given Gx
    void selectCharset(CharsetSet set, CharsetDesignator designator);
    void setCharSet(CharsetSet set);
    CharsetSet charSet() const;

    CharsetDesignator activeCharset() const;

    // Charset designators for each Gx bank
    CharsetDesignator g0() const;
    CharsetDesignator g1() const;
    CharsetDesignator g2() const;
    CharsetDesignator g3() const;

    // Translate a character using the currently active charset
    char32_t applyCharset(char32_t ch) const;
    char32_t resolveCharsetChar(char32_t ch) const;

    // Expose charset translation map for a given designator
    const std::unordered_map<char32_t, char32_t> &getMap(CharsetDesignator designator) const;
    // Expose a helper to map a char for a specific charset
    char32_t mapCharForCharset(char32_t ch, CharsetDesignator charset) const;

private:
    // Charset map fetcher for a specific Gx
    CharsetDesignator getDesignatorFor(CharsetSet set) const;

    CharsetSet                                              m_charSet = CharsetSet::G0;
    CharsetDesignator                                       m_g0 = CharsetDesignator::US_ASCII;
    CharsetDesignator                                       m_g1 = CharsetDesignator::US_ASCII;
    CharsetDesignator                                       m_g2 = CharsetDesignator::US_ASCII;
    CharsetDesignator                                       m_g3 = CharsetDesignator::US_ASCII;
    // Charset translation maps
    static const std::unordered_map<char32_t, char32_t>     DEC_SPECIAL_GRAPHICS;
    static const std::unordered_map<char32_t, char32_t>     DEC_SUPPLEMENTAL;
    static const std::unordered_map<char32_t, char32_t>     DEC_TECHNICAL;
};

}


