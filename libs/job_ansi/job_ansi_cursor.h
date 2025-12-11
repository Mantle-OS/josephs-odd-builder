#pragma once

#include <string>
#include <algorithm>

#include "utils/job_ansi_enums.h"
#include "utils/rgb_color.h"

namespace job::ansi {
using namespace job::ansi::utils::enums;
using job::ansi::utils::RGBColor;

class Cursor {
public:
    Cursor(int maxRows, int maxCols):
        m_maxRows(maxRows),
        m_maxCols(maxCols)
    {
    }

    int row() const
    {
        return m_row;
    }

    int col() const
    {
        return m_col;
    }

    void setPosition(int r, int c)
    {
        m_row = std::clamp(r, 0, m_maxRows - 1);
        m_col = std::clamp(c, 0, m_maxCols - 1);
    }

    // Movement
    void move(int rowDelta, int colDelta)
    {
        m_row = std::clamp(m_row + rowDelta, 0, m_maxRows - 1);
        m_col = std::clamp(m_col + colDelta, 0, m_maxCols - 1);
    }

    // State management
    void save()
    {
        m_savedRow = m_row;
        m_savedCol = m_col;
        m_savedStyle = m_style;
        m_savedCustomColor = m_customColor;
        m_savedVisible = m_visible;
        m_savedBlinking = m_blinking;
    }

    void restore()
    {
        if (m_savedRow >= 0 && m_savedCol >= 0) {
            m_row = m_savedRow;
            m_col = m_savedCol;
            m_style = m_savedStyle;
            m_customColor = m_savedCustomColor;
            m_visible = m_savedVisible;
            m_blinking = m_savedBlinking;
        }
    }

    void setStyle(int value)
    {
        if (value >= 0 && value <= 6)
            m_style = static_cast<CursorStyle>(value);
    }

    CursorStyle cursorStyle() const
    {
        return m_style;
    }

    void setDefaultCursorColor(const RGBColor &color)
    {
        m_defaultColor = color;
    }

    RGBColor effectiveCursorColor() const
    {
        return m_customColor.value_or(m_defaultColor);
    }

    const std::optional<RGBColor> &customColor() const
    {
        return m_customColor;
    }

    const RGBColor &defaultColor() const
    {
        return m_defaultColor;
    }

    void setCursorColor(const std::string &hex)
    {
        auto maybeColor = job::ansi::utils::parseColorString(hex);
        if (maybeColor)
            m_customColor = *maybeColor;
    }

    void resetCursorColor()
    {
        m_customColor.reset();
    }

    // Cursor advancement
    void advance(int maxCols)
    {
        ++m_col;
        if (m_col >= maxCols) {
            m_col = 0;
            ++m_row;
            m_row = std::clamp(m_row, 0, m_maxRows - 1);
        }
    }

    bool visible() const
    {
        return m_visible;
    }

    void setVisible(bool visible)
    {
        m_visible = visible;
    }

    bool isBlinking() const
    {
        return m_blinking;
    }

    void setBlinking(bool blink)
    {
        m_blinking = blink;
    }

    void set(int row, int col)
    {
        m_row = std::clamp(row, 0, m_maxRows - 1);
        m_col = std::clamp(col, 0, m_maxCols - 1);
    }

    bool isBlockMode() const
    {
        return m_style == CursorStyle::SteadyBlock || m_style == CursorStyle::BlinkingBlock;
    }

    bool isUnderlineMode() const
    {
        return m_style == CursorStyle::SteadyUnderline || m_style == CursorStyle::BlinkingUnderline;
    }

    bool isBarMode() const
    {
        return m_style == CursorStyle::SteadyBar || m_style == CursorStyle::BlinkingBar;
    }

    bool isInverseMode() const
    {
        return isBlockMode(); // Block mode is the only one that fully inverts background
    }

private:
    int                         m_maxRows;
    int                         m_maxCols;
    int                         m_row               = 0;
    int                         m_col               = 0;
    bool                        m_visible           = true;
    bool                        m_blinking          = true;
    CursorStyle                 m_style             = CursorStyle::SteadyBlock;
    int                         m_savedRow          = -1;
    int                         m_savedCol          = -1;
    bool                        m_savedVisible      = false;
    bool                        m_savedBlinking     = true;
    CursorStyle                 m_savedStyle        = CursorStyle::SteadyBlock;
    RGBColor                    m_defaultColor      = RGBColor(0, 255, 0); // green
    std::optional<RGBColor>     m_customColor;
    std::optional<RGBColor>     m_savedCustomColor;
};


}
