#include "charset_translator.h"

namespace job::ansi::utils {

// using CharsetDesignator;
using CharsetSet = CharsetTranslator::CharsetSet;

const std::unordered_map<char32_t, char32_t> CharsetTranslator::DEC_TECHNICAL {
    { 0x21, U'⎷' }, { 0x22, U'┌' }, { 0x23, U'─' }, { 0x24, U'⌠' }, { 0x25, U'⌡' },
    { 0x26, U'│' }, { 0x27, U'⎡' }, { 0x28, U'⎣' }, { 0x29, U'⎤' }, { 0x2A, U'⎦' },
    { 0x2B, U'⎛' }, { 0x2C, U'⎝' }, { 0x2D, U'⎞' }, { 0x2E, U'⎠' }, { 0x2F, U'⎨' },
    { 0x30, U'⎬' }, { 0x31, U'⎲' }, { 0x32, U'⎳' }, { 0x33, U'╲' }, { 0x34, U'╱' },
    { 0x35, U'⌝' }, { 0x36, U'⌟' }, { 0x37, U'⟩' },
    { 0x3C, U'≤' }, { 0x3D, U'≠' }, { 0x3E, U'≥' }, { 0x3F, U'∫' },
    { 0x40, U'∴' }, { 0x41, U'∝' }, { 0x42, U'∞' }, { 0x43, U'÷' }, { 0x44, U'Δ' },
    { 0x45, U'∇' }, { 0x46, U'Φ' }, { 0x47, U'Γ' }, { 0x48, U'∼' }, { 0x49, U'≃' },
    { 0x4A, U'Θ' }, { 0x4B, U'×' }, { 0x4C, U'Λ' }, { 0x4D, U'⇔' }, { 0x4E, U'⇒' },
    { 0x4F, U'≡' }, { 0x50, U'Π' }, { 0x51, U'Ψ' }, { 0x52, U'Σ' }, { 0x54, U'√' },
    { 0x55, U'Ω' }, { 0x56, U'Ξ' }, { 0x57, U'Υ' }, { 0x58, U'⊂' }, { 0x59, U'⊃' },
    { 0x5A, U'∩' }, { 0x5B, U'∪' }, { 0x5C, U'∧' }, { 0x5D, U'∨' }, { 0x5E, U'¬' },
    { 0x5F, U'α' }, { 0x60, U'β' }, { 0x61, U'χ' }, { 0x62, U'δ' }, { 0x63, U'ε' },
    { 0x64, U'φ' }, { 0x65, U'γ' }, { 0x66, U'η' }, { 0x67, U'ι' }, { 0x68, U'θ' },
    { 0x69, U'κ' }, { 0x6A, U'λ' }, { 0x6C, U'ν' }, { 0x6D, U'∂' },
    { 0x6E, U'π' }, { 0x6F, U'ψ' }, { 0x70, U'ρ' }, { 0x71, U'σ' }, { 0x72, U'τ' },
    { 0x74, U'ƒ' }, { 0x75, U'ω' }, { 0x76, U'ξ' }, { 0x77, U'υ' }, { 0x78, U'ζ' },
    { 0x79, U'←' }, { 0x7A, U'↑' }, { 0x7B, U'→' }, { 0x7C, U'↓' }
};

const std::unordered_map<char32_t, char32_t> CharsetTranslator::DEC_SPECIAL_GRAPHICS {
    {U'`', U'◆'},  // diamond
    {U'a', U'▒'},  // checkerboard
    {U'b', U'␉'},  // HT (horizontal tab)
    {U'c', U'␌'},  // FF (form feed)
    {U'd', U'␍'},  // CR (carriage return)
    {U'e', U'␊'},  // LF (line feed)
    {U'f', U'°'},  // degree
    {U'g', U'±'},  // plus-minus
    {U'h', U'⎺'},  // top bar
    {U'i', U'⎻'},  // bottom bar
    {U'j', U'┘'},  // lower right corner
    {U'k', U'┐'},  // upper right corner
    {U'l', U'┌'},  // upper left corner
    {U'm', U'└'},  // lower left corner
    {U'n', U'┼'},  // plus / intersection
    {U'o', U'⎯'},  // horizontal scan line 1
    {U'p', U'⎯'},  // horizontal scan line 3 (approximation)
    {U'q', U'─'},  // horizontal line
    {U'r', U'⎯'},  // horizontal scan line 7 (approximation)
    {U's', U'⎯'},  // horizontal scan line 9 (approximation)
    {U't', U'├'},  // vertical and right
    {U'u', U'┤'},  // vertical and left
    {U'v', U'┴'},  // vertical and up
    {U'w', U'┬'},  // vertical and down
    {U'x', U'│'},  // vertical line
    {U'y', U'≤'},  // less than or equal
    {U'z', U'≥'},  // greater than or equal
    {U'{', U'π'},  // pi
    {U'|', U'≠'},  // not equal
    {U'}', U'£'},  // pound
    {U'0', U'▮'}, // ▮
    {U'~', U'·'}   // bullet / centered dot
};

const std::unordered_map<char32_t, char32_t> CharsetTranslator::DEC_SUPPLEMENTAL = {
    {U'!', U'¡'},  // Inverted exclamation
    {U'$', U'¤'},  // Currency sign
    {U'%', U'µ'},  // Micro sign
    {U'&', U'§'},  // Section sign
    {U'(', U'©'},  // Copyright
    {U')', U'®'},  // Registered trademark
    {U'*', U'™'},  // Trademark
    {U'+', U'±'},  // Plus-minus
    {U',', U'¼'},  // One quarter
    {U'-', U'½'},  // One half
    {U'.', U'¾'},  // Three quarters
    {U'/', U'÷'},  // Division
    {U':', U'∶'},   // Ratio
    {U'=', U'≠'},  // Not equal
    {U'>', U'≥'},  // Greater than or equal
    {U'<', U'≤'},  // Less than or equal
    {U'?', U'¿'},  // Inverted question
    {U'@', U'€'}   // Euro sign
};

CharsetTranslator::CharsetTranslator() = default;

void CharsetTranslator::selectCharset(CharsetSet set, CharsetDesignator designator)
{
    switch (set) {
    case CharsetSet::G0:
        m_g0 = designator;
        break;
    case CharsetSet::G1:
        m_g1 = designator;
        break;
    case CharsetSet::G2:
        m_g2 = designator;
        break;
    case CharsetSet::G3:
        m_g3 = designator;
        break;
    }
}

void CharsetTranslator::setCharSet(CharsetSet set)
{
    m_charSet = set;
}

CharsetSet CharsetTranslator::charSet() const
{
    return m_charSet;
}

CharsetDesignator CharsetTranslator::activeCharset() const
{
    CharsetDesignator ret;
    switch (m_charSet) {
    case CharsetSet::G0:
        ret = g0();
        break;
    case CharsetSet::G1:
        ret = g1();
        break;
    case CharsetSet::G2:
        ret = g2();
        break;
    case CharsetSet::G3:
        ret = g3();
        break;
    }
    return ret;
}

CharsetDesignator CharsetTranslator::g0() const
{
    return m_g0;
}

CharsetDesignator CharsetTranslator::g1() const
{
    return m_g1;
}

CharsetDesignator CharsetTranslator::g2() const
{
    return m_g2;
}

CharsetDesignator CharsetTranslator::g3() const
{
    return m_g3;
}

char32_t CharsetTranslator::applyCharset(char32_t ch) const
{
    const auto &map = getMap(getDesignatorFor(m_charSet));
    auto it = map.find(ch);
    return it != map.end() ? it->second : ch;
}

char32_t CharsetTranslator::resolveCharsetChar(char32_t ch) const
{
    return applyCharset(ch);
}

CharsetDesignator CharsetTranslator::getDesignatorFor(CharsetSet set) const
{
    switch (set) {
    case CharsetSet::G0:
        return m_g0;
    case CharsetSet::G1:
        return m_g1;
    case CharsetSet::G2:
        return m_g2;
    case CharsetSet::G3:
        return m_g3;
    default:
        return CharsetDesignator::UNUSED;
    }
}

const std::unordered_map<char32_t, char32_t> &CharsetTranslator::getMap(CharsetDesignator designator) const
{
    switch (designator) {
    case CharsetDesignator::DEC_SPECIAL_GRAPHICS:
        return DEC_SPECIAL_GRAPHICS;
    case CharsetDesignator::DEC_SUPPLEMENTAL:
        return DEC_SUPPLEMENTAL;
    case CharsetDesignator::DEC_TECHNICAL:
        return DEC_TECHNICAL;
    default:
        static const std::unordered_map<char32_t, char32_t> empty;
        return empty;
    }
}

char32_t CharsetTranslator::mapCharForCharset(char32_t ch, CharsetDesignator charset) const
{
    const auto &map = getMap(charset);
    auto it = map.find(ch);
    return it != map.end() ? it->second : ch;
}

}
