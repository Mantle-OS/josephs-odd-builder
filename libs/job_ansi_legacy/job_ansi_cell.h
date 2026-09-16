#pragma once

#include <memory>
#include <string>
#include <vector>

#include "job_ansi_attributes.h"
#include "job_ansi_line_display_mode.h"
#include "utils/rgb_color.h"

namespace job::ansi {
using job::ansi::utils::RGBColor;

class Cell {
public:
    using Line =  std::vector<Cell>;

    Cell();
    explicit Cell(char32_t ch);
    Cell(char32_t ch, Attributes::Ptr attr, int width = 1) noexcept;
    Cell(const Cell &other);
    Cell(Cell &&other) noexcept;

    Cell &operator=(const Cell &other);
    Cell &operator=(Cell &&other) noexcept = default;


    [[nodiscard]] char32_t getChar() const noexcept;
    void setChar(char32_t ch, int width = 1);

    [[nodiscard]] int width() const noexcept;
    void setWidth(int width) noexcept{m_charWidth = width;}

    [[nodiscard]] bool isTrailing() const noexcept;
    void setTrailing(bool trailing);

    [[nodiscard]] std::u32string marks() const;
    void clearMarks();
    void appendCombiningChar(char32_t ch);
    [[nodiscard]] std::u32string fullCharSequence() const;
    [[nodiscard]] bool hasMarks() const noexcept;


    // tracking for painting in other places.
    [[nodiscard]] bool isRenderable() const noexcept;
    [[nodiscard]] bool isEmpty() const;

    [[nodiscard]] bool dirty() const noexcept;
    void setDirty(bool dirty);

    // Shared default blank cell with default attributes
    static const Cell &blank();

    // Shared default attributes (used for flyweight pattern)
    [[nodiscard]] static Attributes::Ptr defaultAttributes();
    [[nodiscard]] const Attributes &attributesOrDefault() const noexcept;
    [[nodiscard]] const Attributes *attributes() const noexcept;
    void setAttributes(const Attributes &attr);

    void reset() noexcept;
    void resetAttributes() noexcept;

    void setForeground(const RGBColor &color);
    void setBackground(const RGBColor &color);
    void resetColors();

    void setLineDisplayMode(LineDisplayMode mode) noexcept;
    [[nodiscard]] LineDisplayMode getLineDisplayMode() const noexcept;

    void setLineWidth(int width) noexcept;
    [[nodiscard]] int getLineWidth() const noexcept;

    void setLineHeight(int height) noexcept;
    [[nodiscard]] int getLineHeight() const noexcept;

    void setLineHeightPosition(bool isTop) noexcept;
    [[nodiscard]] bool isTopHalf() const noexcept;

    void setProtection(bool erase, bool write) noexcept;
    [[nodiscard]] bool isProtectedForErase() const noexcept;
    [[nodiscard]] bool isProtectedForWrite() const noexcept;

private:
    Attributes::Ptr                     m_attr;
    std::unique_ptr<std::u32string>     m_marks;
    char32_t                            m_char = U' ';


    uint32_t m_charWidth     : 4 = 1; // Supports widths 0-15
    uint32_t m_lineWidth     : 4 = 1;
    uint32_t m_lineHeight    : 4 = 1;
    uint32_t m_lineMode      : 3 = 0; // Enum LineDisplayMode

    uint32_t m_dirty         : 1 = 1;
    uint32_t m_trailing      : 1 = 0;
    uint32_t m_isTopHalf     : 1 = 0;
    uint32_t m_protectErase  : 1 = 0;
    uint32_t m_protectWrite  : 1 = 0;
};

} // namespace Core
