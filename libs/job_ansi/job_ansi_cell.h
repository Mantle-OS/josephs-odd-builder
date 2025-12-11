#pragma once

#include <memory>
#include <string>
#include "job_ansi_attributes.h"
#include "utils/rgb_color.h"

namespace job::ansi {

using job::ansi::utils::RGBColor;

// Line display modes for DECSWL, DECDWL, DECDHL
enum class LineDisplayMode {
    SingleWidth,         // Normal width (DECSWL)
    DoubleWidth,        // Double width (DECDWL)
    DoubleWidthTop,     // Double width, top half (DECDHL)
    DoubleWidthBottom,  // Double width, bottom half (DECDHL)
    DoubleHeight,       // Double height (DECDHL)
    DoubleHeightTop,    // Double height, top half (DECDHL)
    DoubleHeightBottom  // Double height, bottom half (DECDHL)
};

class Cell {
public:
    Cell();
    explicit Cell(char32_t ch);
    Cell(char32_t ch, Attributes::Ptr attr);
    Cell(const Cell &other);

    Cell &operator=(const Cell &other);

    char32_t getChar() const;
    void setChar(char32_t ch);

    int width() const;

    bool isTrailing() const;
    void setTrailing(bool trailing);

    std::u32string marks() const;
    void clearMarks();
    void appendCombiningChar(char32_t ch);

    // tracking for painting in other places.
    bool isRenderable() const;
    bool isEmpty() const;

    bool dirty() const;
    void setDirty(bool dirty);

    std::u32string fullCharSequence() const;

    // Shared default blank cell with default attributes
    static const Cell &blank();

    // Shared default attributes (used for flyweight pattern)
    static Attributes::Ptr defaultAttributes();
    const Attributes &attributesOrDefault() const;
    void setAttributes(const Attributes &attr);
    const Attributes *attributes() const;

    void reset();
    void resetAttributes();


    void setForeground(const RGBColor &color);
    void setBackground(const RGBColor &color);
    void resetColors();

    void setLineDisplayMode(LineDisplayMode mode);
    LineDisplayMode getLineDisplayMode() const;

    void setLineWidth(int width);
    int getLineWidth() const;

    void setLineHeight(int height);
    int getLineHeight() const;

    void setLineHeightPosition(bool isTop);
    bool isTopHalf() const;

    void setProtection(bool erase, bool write);
    bool isProtectedForErase() const;
    bool isProtectedForWrite() const;

private:
    bool                    m_dirty = true;
    Attributes::Ptr         m_attr;
    std::u32string          m_marks;
    char32_t                m_char = U' ';
    bool                    m_trailing = false;
    int                     m_charWidth = 1;
    LineDisplayMode         m_lineMode = LineDisplayMode::SingleWidth;
    int                     m_lineWidth = 1;
    int                     m_lineHeight = 1;
    bool                    m_isTopHalf = false;
    bool                    m_protectErase = false;
    bool                    m_protectWrite = false;
};

} // namespace Core
