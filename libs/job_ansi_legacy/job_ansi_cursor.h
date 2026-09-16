#pragma once

#include <string>
#include <memory>

#include "utils/job_ansi_enums.h"
#include "utils/rgb_color.h"

namespace job::ansi {
using namespace job::ansi::utils::enums;
using job::ansi::utils::RGBColor;

class Cursor {
public:
    using Ptr  = std::shared_ptr<Cursor>;
    using UPtr = std::unique_ptr<Cursor>;

    Cursor() = default;
    Cursor(int maxRows, int maxCols);
    Cursor(const Cursor&) = default;
    Cursor(Cursor&&) noexcept = default;
    Cursor &operator=(const Cursor&) = default;
    Cursor &operator=(Cursor&&) noexcept = default;
    ~Cursor() = default;

    [[nodiscard]] int row() const noexcept;
    [[nodiscard]] int column() const noexcept;

    void setPosition(int row, int col) noexcept;
    void updateDimensions(int rows, int cols) noexcept
    {
        m_maxRows = rows;
        m_maxCols = cols;
        setPosition(m_row, m_column);
    }

    void move(int rowDelta, int colDelta) noexcept;
    void advance(int maxCols) noexcept;
    void up(int count = 1) noexcept;
    void down(int count = 1) noexcept;
    void left(int count = 1) noexcept;
    void right(int count = 1) noexcept;


    // State management
    void save() noexcept;
    void restore() noexcept;

    void setStyle(int value) noexcept;
    [[nodiscard]] CursorStyle cursorStyle() const noexcept;
    void setDefaultCursorColor(const RGBColor &color) noexcept;

    [[nodiscard]] RGBColor effectiveCursorColor() const noexcept;
    [[nodiscard]] const std::optional<RGBColor> &customColor() const noexcept;
    [[nodiscard]] const RGBColor &defaultColor() const noexcept;
    void setCursorColor(const std::string &hex);
    void resetCursorColor() noexcept;

    [[nodiscard]] bool visible() const noexcept;
    void setVisible(bool visible) noexcept;

    [[nodiscard]] bool isBlinking() const noexcept;
    void setBlinking(bool blink) noexcept;

    [[nodiscard]] bool isBlockMode() const noexcept;
    [[nodiscard]] bool isUnderlineMode() const noexcept;
    [[nodiscard]] bool isBarMode() const noexcept;
    [[nodiscard]] bool isInverseMode() const noexcept;

private:
    int                         m_maxRows;
    int                         m_maxCols;
    int                         m_row               = 0;
    int                         m_column            = 0;
    bool                        m_visible           = true;
    bool                        m_blinking          = true;
    CursorStyle                 m_style             = CursorStyle::SteadyBlock;
    int                         m_savedRow          = -1;
    int                         m_savedCol          = -1;
    bool                        m_savedVisible      = false;
    bool                        m_savedBlinking     = true;
    CursorStyle                 m_savedStyle        = CursorStyle::SteadyBlock;
    RGBColor                    m_defaultColor      = RGBColor(0, 255, 0);
    std::optional<RGBColor>     m_customColor;
    std::optional<RGBColor>     m_savedCustomColor;
};

}
