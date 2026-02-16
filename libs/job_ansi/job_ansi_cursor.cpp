#include "job_ansi_cursor.h"
#include <algorithm>
namespace job::ansi {
Cursor::Cursor(int maxRows, int maxCols) :
    m_maxRows(maxRows),
    m_maxCols(maxCols)
{}

int Cursor::row() const noexcept
{
    return m_row;
}

int Cursor::column() const noexcept
{
    return m_column;
}

void Cursor::setPosition(int row, int col) noexcept
{
    m_row = std::clamp(row, 0, m_maxRows - 1);
    m_column = std::clamp(col, 0, m_maxCols - 1);
}

void Cursor::move(int rowDelta, int colDelta) noexcept
{
    m_row = std::clamp(m_row + rowDelta, 0, m_maxRows - 1);
    m_column = std::clamp(m_column + colDelta, 0, m_maxCols - 1);
}

void Cursor::save() noexcept
{
    m_savedRow = m_row;
    m_savedCol = m_column;
    m_savedStyle = m_style;
    m_savedCustomColor = m_customColor;
    m_savedVisible = m_visible;
    m_savedBlinking = m_blinking;
}

void Cursor::restore() noexcept
{
    if (m_savedRow >= 0 && m_savedCol >= 0) {
        m_row = m_savedRow;
        m_column = m_savedCol;
        m_style = m_savedStyle;
        m_customColor = m_savedCustomColor;
        m_visible = m_savedVisible;
        m_blinking = m_savedBlinking;
    }
}

void Cursor::setStyle(int value) noexcept
{
    if (value >= 0 && value <= 6)
        m_style = static_cast<CursorStyle>(value);
}

void Cursor::setDefaultCursorColor(const RGBColor &color) noexcept
{
    m_defaultColor = color;
}

void Cursor::setCursorColor(const std::string &hex)
{
    auto maybeColor = job::ansi::utils::parseColorString(hex);
    if (maybeColor)
        m_customColor = *maybeColor;
}

void Cursor::resetCursorColor() noexcept
{
    m_customColor.reset();
}

void Cursor::advance(int maxCols) noexcept
{
    ++m_column;
    if (m_column >= maxCols) {
        m_column = 0;
        ++m_row;
        m_row = std::clamp(m_row, 0, m_maxRows - 1);
    }
}

void Cursor::up(int count) noexcept
{
    move(-count, 0);
}

void Cursor::down(int count) noexcept
{
    move(count, 0);
}

void Cursor::left(int count) noexcept
{
    move(0, -count);
}

void Cursor::right(int count) noexcept
{
    move(0, count);
}

bool Cursor::visible() const noexcept
{
    return m_visible;
}

void Cursor::setVisible(bool visible) noexcept
{
    m_visible = visible;
}

bool Cursor::isBlinking() const noexcept
{
    return m_blinking;
}

void Cursor::setBlinking(bool blink) noexcept
{
    m_blinking = blink;
}

bool Cursor::isBlockMode() const noexcept
{
    return m_style == CursorStyle::SteadyBlock || m_style == CursorStyle::BlinkingBlock;
}

bool Cursor::isUnderlineMode() const noexcept
{
    return m_style == CursorStyle::SteadyUnderline || m_style == CursorStyle::BlinkingUnderline;
}

bool Cursor::isBarMode() const noexcept
{
    return m_style == CursorStyle::SteadyBar || m_style == CursorStyle::BlinkingBar;
}

bool Cursor::isInverseMode() const noexcept
{
    return isBlockMode(); // Block mode is the only one that fully inverts background
}


const RGBColor &Cursor::defaultColor() const noexcept
{
    return m_defaultColor;
}

const std::optional<RGBColor> &Cursor::customColor() const noexcept
{
    return m_customColor;
}

RGBColor Cursor::effectiveCursorColor() const noexcept
{
    return m_customColor.value_or(m_defaultColor);
}

CursorStyle Cursor::cursorStyle() const noexcept
{
    return m_style;
}
}
