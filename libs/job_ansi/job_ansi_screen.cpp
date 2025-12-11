#include "job_ansi_screen.h"
#include <algorithm>
#include <memory>
#include <iostream>
#include <cassert>

namespace job::ansi {

const Cell kDefaultCell{};
Screen::Screen(int rows, int cols)
{
    resize(rows, cols);
    // Set default tab stops every 8 columns, up to m_columns - 1
    for (int col = 0; col < m_columns; col += 8)
        setTabStop(col);

    m_currentAttributes = Attributes::intern(Attributes());
}

void Screen::insertLines(int startRow, int count)
{
    if (startRow < 0 || startRow >= m_rows || count <= 0)
        return;

    int endRow = m_scrollBottom;
    if (startRow >= endRow)
        return;

    count = std::min(count, endRow - startRow);
    auto &buffer = activeBuffer();
    int width = m_columns;

    // Move existing lines down
    for (int row = endRow - 1; row >= startRow + count; --row)
        for (int col = 0; col < width; ++col)
            buffer[index(row, col)] = buffer[index(row - count, col)];


    // Clear new inserted lines
    int clearLimit = std::min(startRow + count, endRow);
    for (int row = startRow; row < clearLimit; ++row)
        for (int col = 0; col < width; ++col)
            buffer[index(row, col)].reset();  // or = Cell{};


    requestRender();
}

void Screen::deleteLines(int count)
{
    deleteLines(m_cursor->row(), count);
}

void Screen::deleteLines(int startRow, int count)
{
    if (startRow < m_scrollTop || startRow >= m_scrollBottom)
        return;

    int linesToMove = m_scrollBottom - startRow - count;
    if (linesToMove > 0)
        for (int row = 0; row < linesToMove; ++row)
            copyRow(startRow + row + count, startRow + row);


    // Clear the bottom lines (respecting protected erase)
    for (int row = m_scrollBottom - count; row < m_scrollBottom; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            Cell &cell = activeBuffer()[index(row, col)];
            if (!cell.isProtectedForErase())
                cell = Cell::blank();
        }
    }

    requestRender();
}

void Screen::clearRow(int row)
{
    if (row < 0 || row >= m_rows)
        return;

    auto &buffer = activeBuffer();
    for (int col = 0; col < m_columns; ++col) {
        auto &cell = buffer[index(row, col)];
        if (!cell.isProtectedForErase())
            cell.reset();
    }

    requestRender();
}

void Screen::copyRow(int from, int to)
{
    if (from < 0 || from >= m_rows || to < 0 || to >= m_rows)
        return;

    auto &buffer = activeBuffer();
    for (int col = 0; col < m_columns; ++col) {
        const Cell &src = buffer[index(from, col)];
        Cell &dst = buffer[index(to, col)];

        if (!dst.isProtectedForWrite()) {
            dst = src;
            markCellDirty(to, col);
        }
    }

    requestRender();
}


void Screen::markCellDirty(int row, int col)
{
    m_dirtyRows.insert(row);
    m_dirtyCols[row].insert(col);
    requestRender();
}

void Screen::clearDirty()
{
    m_dirtyRows.clear();
    m_dirtyCols.clear();
    m_needsRender = false;
}

const std::unordered_set<int> &Screen::dirtyRows() const
{
    return m_dirtyRows;
}

const std::unordered_set<int> &Screen::dirtyCols(int row) const
{
    auto it = m_dirtyCols.find(row);
    if (it != m_dirtyCols.end())
        return it->second;
    static const std::unordered_set<int> empty;
    return empty;
}

void Screen::resize(int rows, int cols)
{
    int oldRows = m_rows;
    int oldCols = m_columns;

    const auto &oldBuffer = isAlternateBufferActive() ? m_altBuffer : m_mainBuffer;

    m_rows = rows;
    m_columns = cols;

    std::vector<Cell> newBuffer(rows * cols, Cell{});

    if (!oldBuffer.empty()) {
        int copyRows = std::min(rows, oldRows);
        int copyCols = std::min(cols, oldCols);

        for (int row = 0; row < copyRows; ++row) {
            for (int col = 0; col < copyCols; ++col) {
                int oldIndex = row * oldCols + col;
                int newIndex = row * cols + col;
                newBuffer[newIndex] = oldBuffer[oldIndex];
            }
        }
    }

    // Set active buffer
    if (isAlternateBufferActive()) {
        m_altBuffer = std::move(newBuffer);
        if (m_mainBuffer.size() != static_cast<size_t>(rows * cols))
            m_mainBuffer.resize(rows * cols, Cell{});
    } else {
        m_mainBuffer = std::move(newBuffer);
        if (m_altBuffer.size() != static_cast<size_t>(rows * cols))
            m_altBuffer.resize(rows * cols, Cell{});
    }

    m_cursor = std::make_unique<Cursor>(rows, cols);
    m_savedCursor = std::make_unique<Cursor>(rows, cols);

    m_tabStops.reset();
    for (int col = 0; col < cols; col += 8)
        setTabStop(col);

    m_scrollBottom = rows;
    requestRender();
}


void Screen::putChar(char32_t ch)
{
    putChar(ch, currentAttributes());
}
// hello there
void Screen::putChar(char32_t ch, Attributes *style)
{
    // Handle control characters
    if (ch == U'\r') {
        moveCursor(m_cursor->row(), 0);
        if (get_autoLinefeed()) {
            if (m_cursor->row() + 1 >= m_scrollBottom) {
                scrollUp();
            }
            moveCursor(std::min(m_cursor->row() + 1, m_scrollBottom - 1), 0);
        }
        return;
    } else if (ch == U'\n') {
        if (m_cursor->row() + 1 >= m_scrollBottom) {
            scrollUp();
        }
        moveCursor(std::min(m_cursor->row() + 1, m_scrollBottom - 1), m_cursor->col());
        return;
    } else if (ch == U'\b') {
        if (m_cursor->col() > 0)
            moveCursor(m_cursor->row(), m_cursor->col() - 1);
        return;
    }

    // Skip out-of-bounds writes
    if (m_cursor->row() < 0 || m_cursor->row() >= m_rows ||
        m_cursor->col() < 0 || m_cursor->col() >= m_columns)
        return;

    ch = applyCharset(ch);

    auto &buffer = activeBuffer();
    const int row = m_cursor->row();
    const int col = m_cursor->col();

    // Insert Mode: shift characters right before writing
    if (get_insertMode()) {
        for (int i = m_columns - 2; i >= col; --i) {
            buffer[index(row, i + 1)] = buffer[index(row, i)];
            markCellDirty(row, i + 1);
        }
        buffer[index(row, col)].reset();
    }

    auto &cell = buffer[index(row, col)];

    // Respect write protection
    if (cell.isProtectedForWrite())
        return;

    // Write character and style
    cell.setChar(ch);
    cell.setAttributes(*style);

    // Apply protection from DECSCA mode
    switch (m_protectionMode) {
    case DECSCA_MODE::ERASE_PROTECTED:
        cell.setProtection(true, false);
        break;
    case DECSCA_MODE::WRITE_ERASE_PROTECTED:
        cell.setProtection(true, true);
        break;
    case DECSCA_MODE::NONE:
    default:
        cell.setProtection(false, false);
        break;
    }

    markCellDirty(row, col);

    // Handle cursor advancement with wraparound
    if (col == m_columns - 1) {
        if (get_wraparound()) {
            if (row + 1 >= m_scrollBottom) {
                scrollUp();
                m_cursor->set(m_scrollBottom - 1, 0);
            } else {
                m_cursor->set(row + 1, 0);
            }
        }
        // else: cursor stays at last column
    } else {
        m_cursor->move(0, 1);
    }

    requestRender();
}


std::vector<Cell> &Screen::activeBuffer()
{
    return m_inAlternateBuffer ? m_altBuffer : m_mainBuffer;
}

const std::vector<Cell> &Screen::activeBuffer() const
{
    return m_inAlternateBuffer ? m_altBuffer : m_mainBuffer;
}

void Screen::enterAlternateBuffer()
{
    m_inAlternateBuffer = true;
    std::fill(m_altBuffer.begin(), m_altBuffer.end(), Cell{});
    setScrollRegion(0, m_rows);  // Reset to full screen
    requestRender();
}

void Screen::exitAlternateBuffer()
{
    m_inAlternateBuffer = false;
    requestRender();
}

bool Screen::isAlternateBufferActive() const
{
    return m_inAlternateBuffer;
}

Cell *Screen::cellAt(int row, int col)
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_columns)
        return nullptr;
    return &activeBuffer()[index(row, col)];
}

void Screen::setScrollRegion(int top, int bottom)
{
    m_scrollTop = std::clamp(top, 0, m_rows - 1);
    m_scrollBottom = std::clamp(bottom, m_scrollTop + 1, m_rows);
}

void Screen::scrollUp(int top, int bottom, int lines)
{

    // std::cerr << "[Scroll] scrollUp called\n";

    if (lines <= 0)
        return;

    if (top < 0)
        top = 0;

    if (bottom > m_rows)
        bottom = m_rows;

    if (top >= bottom)
        return;

    assert(top >= 0 && bottom <= m_rows);
    assert(lines <= (bottom - top));

    auto &buffer = activeBuffer();
    int width = m_columns;

    for (int i = 0; i < lines; ++i) {
        std::vector<Cell> scrolledRow;
        scrolledRow.reserve(width);
        for (int col = 0; col < width; ++col)
            scrolledRow.push_back(buffer[index(top, col)]);

        m_scrollback.push_back(std::move(scrolledRow));

        if (m_scrollback.size() > MAX_SCROLLBACK_LINES) {
            m_scrollback.pop_front();
            if (m_scrollOffset > 0) {
                m_scrollOffset = std::max(0, m_scrollOffset - 1);
            }
        }
    }

    // Move lines up
    for (int row = top; row < bottom - lines; ++row) {
        for (int col = 0; col < width; ++col) {
            buffer[index(row, col)] = std::move(buffer[index(row + lines, col)]);
            markCellDirty(row, col);
        }
    }

    // Clear lines at the bottom, respecting character protection
    for (int row = bottom - lines; row < bottom; ++row) {
        for (int col = 0; col < width; ++col) {
            auto &cell = buffer[index(row, col)];
            if (!cell.isProtectedForErase()) {
                cell = kDefaultCell;
                markCellDirty(row, col);
            }
        }
    }
    requestRender();

    // Ensure cursor remains within scroll region bounds after scroll
    if (m_cursor->row() >= bottom)
        m_cursor->set(bottom - 1, 0);
    else if (m_cursor->row() < top)
        m_cursor->set(top, 0);

}


void Screen::scrollUp()
{
    std::cerr << "[Scroll] scrollUp called\n";
    scrollUp(m_scrollTop, m_scrollBottom, 1);
}

void Screen::scrollDown(int top, int bottom, int lines)
{
    std::cerr << "[Scroll] scrollDOWN called\n";

    if (lines <= 0)
        return;

    if (top < 0)
        top = 0;

    if (bottom > m_rows)
        bottom = m_rows;

    if (top >= bottom)
        return;

    assert(top >= 0 && bottom <= m_rows);
    assert(lines <= (bottom - top));

    auto &buffer = activeBuffer();
    int width = m_columns;

    // Move lines down
    for (int row = bottom - 1; row >= top + lines; --row) {
        for (int col = 0; col < width; ++col) {
            buffer[index(row, col)] = std::move(buffer[index(row - lines, col)]);
            markCellDirty(row, col);
        }
    }

    // Clear lines at the top, respecting character protection
    for (int row = top; row < top + lines && row < bottom; ++row) {
        for (int col = 0; col < width; ++col) {
            auto &cell = buffer[index(row, col)];
            if (!cell.isProtectedForErase()) {
                cell = kDefaultCell;
                markCellDirty(row, col);
            }
        }
    }

    requestRender();

    // Ensure cursor remains within scroll region bounds after scroll
    if (m_cursor->row() >= bottom)
        m_cursor->set(bottom - 1, 0);
    else if (m_cursor->row() < top)
        m_cursor->set(top, 0);

}

const std::deque<std::vector<Cell>> &Screen::scrollbackLines() const
{
    return m_scrollback;
}

void Screen::clearScrollback()
{
    m_scrollback.clear();
    m_scrollOffset = 0;
    requestRender();
}

void Screen::scrollbackPageUp()
{
    if (!m_scrollback.empty()) {
        m_scrollOffset = std::min<int>(m_scrollOffset + m_rows, m_scrollback.size());
        requestRender();
    }
}

void Screen::scrollbackPageDown()
{
    if (m_scrollOffset > 0) {
        m_scrollOffset = std::max(0, m_scrollOffset - m_rows);
        requestRender();
    }
}

int Screen::scrollOffset() const
{
    // Always clamp to valid range for safety
    if (m_scrollOffset < 0)
        return 0;

    if (m_scrollOffset > (int)m_scrollback.size())
        return (int)m_scrollback.size();

    return m_scrollOffset;
}

void Screen::setScrollOffset(int offset)
{
    // Clamp to valid range
    if (offset < 0)
        offset = 0;

    if (offset > (int)m_scrollback.size())
        offset = (int)m_scrollback.size();

    m_scrollOffset = offset;
}

void Screen::scrollbackLineUp()
{
    if (m_scrollOffset < (int)m_scrollback.size()) {
        m_scrollOffset += 1;
        requestRender();
    }
}

void Screen::scrollbackLineDown()
{
    if (m_scrollOffset > 0) {
        m_scrollOffset -= 1;
        requestRender();
    }
}

bool Screen::isInScrollback() const
{
    return m_scrollOffset > 0;
}

void Screen::moveCursor(int rowDelta, int colDelta)
{
    // std::cerr << "[Cursor] moveCursor(" << rowDelta << ", " << colDelta << ")\n";

    int regionTop = originRow();
    int regionBottom = originBottom();
    int newRow = std::clamp(m_cursor->row() + rowDelta, regionTop, regionBottom - 1);
    int newCol = std::clamp(m_cursor->col() + colDelta, 0, columnCount() - 1);
    setCursor(newRow, newCol); // no longer subtract regionTop
}
void Screen::moveCursorRelative(int rowDelta, int colDelta, int regionTop, int regionBottom)
{
    if (!m_cursor)
        return;

    int baseRow  = regionTop;
    int limitRow = regionBottom - 1;

    int newRow = std::clamp(m_cursor->row() + rowDelta, baseRow, limitRow);
    int newCol = std::clamp(m_cursor->col() + colDelta, 0, columnCount() - 1);

    setCursor(newRow, newCol);  // absolute
}


void Screen::setCursor(int row, int col)
{
    // std::cerr << "[Cursor] setCursor(" << row << ", " << col << ")\n";

    int clampedRow = std::clamp(row, 0, rowCount() - 1);
    int clampedCol = std::clamp(col, 0, columnCount() - 1);
    if (m_cursor)
        m_cursor->set(clampedRow, clampedCol);
    requestRender();
}


void Screen::saveCursor()
{
    m_cursor->save();
}

void Screen::restoreCursor()
{
    m_cursor->restore();
    requestRender();
}

void Screen::setCursorStyle(int style)
{
    m_cursor->setStyle(style);
    requestRender();
}

void Screen::sendMouseSGR(int button, bool pressed, int row, int col)
{
    if(on_mouse_info){
        char action = pressed ? 'M' : 'm';
        int bcode = 32 + button;  // TODO: Map button to xterm spec
        std::string seq = "\033[<" + std::to_string(bcode) +
                          ";" + std::to_string(col + 1) +
                          ";" + std::to_string(row + 1) + action;


        on_mouse_info(seq);  // ← this should be your backend callback
    }
}

void Screen::sendMouseX10(int button, bool, int row, int col)
{
    if(on_mouse_info){
        int bcode = 32 + button;
        int x = 32 + col + 1;
        int y = 32 + row + 1;
        std::string seq = "\033[M";
        seq += static_cast<char>(bcode);
        seq += static_cast<char>(x);
        seq += static_cast<char>(y);
        on_mouse_info(seq);
    }
}

void Screen::sendMouseVT200Highlight(int button, bool pressed, int row, int col)
{
    // VT200 only reports on button press (like X10)
    if (on_mouse_info && pressed)
        sendMouseX10(button, pressed, row, col);
}

void Screen::sendMouseButtonEventMode(int button, bool pressed, int row, int col)
{
    // Same format as X10, but includes both press/release events
    if (on_mouse_info)
        sendMouseX10(button, pressed, row, col);
}

void Screen::sendMouseAnyEventMode(int button, bool pressed, int row, int col)
{
    // Every mouse movement and click triggers a report
    if (on_mouse_info)
        sendMouseX10(button, pressed, row, col);
}

void Screen::sendMouseUtf8Encoding(int button, [[maybe_unused]]bool pressed, int row, int col)
{
    if (on_mouse_info) {
        int bcode = 32 + button;
        int x = 32 + col + 1;
        int y = 32 + row + 1;

        // Encode each byte in UTF-8 (xterm expects bytes >127 for UTF-8 mode)
        std::string seq = "\033[M";
        seq += static_cast<char>(0xC0 | ((bcode >> 6) & 0x1F));
        seq += static_cast<char>(0x80 | (bcode & 0x3F));
        seq += static_cast<char>(0xC0 | ((x >> 6) & 0x1F));
        seq += static_cast<char>(0x80 | (x & 0x3F));
        seq += static_cast<char>(0xC0 | ((y >> 6) & 0x1F));
        seq += static_cast<char>(0x80 | (y & 0x3F));

        on_mouse_info(seq);
    }
}

void Screen::sendFocusEvent(bool focused)
{
    std::string seq = focused ? "\033[I" : "\033[O";
    sendString(seq);
}

void Screen::set_originMode(bool enabled)
{
    m_originMode = enabled;
    if (enabled)
        moveCursor(m_scrollTop, 0);
}

int Screen::originRow() const
{
    return m_originMode ? m_scrollTop : 0;
}

int Screen::originBottom() const
{
    return m_originMode ? m_scrollBottom : m_rows;
}

void Screen::setTabStop(int col)
{
    if (col >= 0 && col < 512)
        m_tabStops.set(col);
}

void Screen::clearTabStop(int col)
{
    if (col >= 0 && col < 512)
        m_tabStops.reset(col);
}

void Screen::clearAllTabStops()
{
    m_tabStops.reset();
}

bool Screen::isTabStop(int col) const
{
    return col >= 0 && col < 512 && m_tabStops.test(col);
}

void Screen::selectCharset(int set, CharsetDesignator designator)
{
    m_charset.selectCharset(static_cast<utils::CharsetTranslator::CharsetSet>(set), designator);
}

char32_t Screen::applyCharset(char32_t ch) const
{
    return m_charset.applyCharset(ch);
}

CharsetDesignator Screen::charsetForCell(int , int ) const
{
    return m_charset.activeCharset();
}

ansi::utils::CharsetDesignator Screen::g0Charset() const
{
    return m_charset.g0();
}

ansi::utils::enums::CharsetDesignator Screen::g1Charset() const
{
    return m_charset.g1();
}

int Screen::activeCharset() const
{
    return static_cast<int>(m_charset.charSet());
}

char32_t Screen::resolveCharsetChar(char32_t ch) const
{
    return m_charset.applyCharset(ch);
}

char32_t Screen::mapCharForCharset(char32_t ch, ansi::utils::enums::CharsetDesignator charset) const
{
    return m_charset.mapCharForCharset(ch, charset);
}

void Screen::eraseToEnd(int row, int col)
{
    for (int r = row; r < rowCount(); ++r) {
        int cStart = (r == row ? col : 0);
        for (int c = cStart; c < columnCount(); ++c) {
            Cell *cell = cellAt(r, c);
            if (cell && !cell->isProtectedForErase()) {
                *cell = Cell::blank();
                markCellDirty(r, c);
            }
        }
    }
    requestRender();
}


void Screen::eraseToStart(int row, int col)
{
    for (int r = 0; r <= row; ++r) {
        int cEnd = (r == row ? col : columnCount() - 1);
        for (int c = 0; c <= cEnd; ++c) {
            Cell *cell = cellAt(r, c);
            if (cell && !cell->isProtectedForErase()) {
                *cell = Cell::blank();
                markCellDirty(r, c);
            }
        }
    }
    requestRender();
}


void Screen::eraseScreen()
{
    clear();
    requestRender();
}

void Screen::eraseScreenAndScrollback()
{
    clearScrollback();
    clear();
    requestRender();
}

void Screen::eraseLineToEnd(int row, int col)
{
    for (int c = col; c < columnCount(); ++c) {
        Cell *cell = cellAt(row, c);
        if (cell && !cell->isProtectedForErase()) {
            *cell = Cell::blank();
            markCellDirty(row, c);
        }
    }
    requestRender();
}
void Screen::eraseLineToStart(int row, int col)
{
    for (int c = 0; c <= col; ++c) {
        Cell *cell = cellAt(row, c);
        if (cell && !cell->isProtectedForErase()) {
            *cell = Cell::blank();
            markCellDirty(row, c);
        }
    }
    requestRender();
}

void Screen::eraseLine(int row)
{
    for (int col = 0; col < columnCount(); ++col) {
        Cell *cell = cellAt(row, col);
        if (cell && !cell->isProtectedForErase()) {
            *cell = Cell::blank();
            markCellDirty(row, col);
        }
    }
    requestRender();
}

void Screen::deleteChars(int count)
{
    int row = m_cursor->row();
    int col = m_cursor->col();
    int maxCol = columnCount();

    int shiftEnd = maxCol - count;
    for (int i = col; i < shiftEnd; ++i) {
        Cell *target = cellAt(row, i);
        Cell *source = cellAt(row, i + count);

        if (target && source && !target->isProtectedForWrite()) {
            *target = *source;
            markCellDirty(row, i);
        }
    }

    for (int i = shiftEnd; i < maxCol; ++i) {
        Cell *cell = cellAt(row, i);
        if (cell && !cell->isProtectedForErase()) {
            cell->reset();
            markCellDirty(row, i);
        }
    }
}


void Screen::eraseChars(int count)
{
    int row = m_cursor->row();
    int col = m_cursor->col();

    for (int i = 0; i < count && (col + i) < columnCount(); ++i) {
        Cell *cell = cellAt(row, col + i);
        if (cell && !cell->isProtectedForErase()) {
            cell->reset();
            markCellDirty(row, col + i);
        }
    }
}


void Screen::repeatLastChar(int count)
{
    if (count <= 0 || !m_cursor)
        return;

    int row = m_cursor->row();
    int col = m_cursor->col();

    for (int i = 0; i < count; ++i) {
        if (col >= columnCount())
            break;

        Cell *cell = cellAt(row, col);
        if (cell && !cell->isProtectedForWrite()) {
            cell->setChar(m_lastCharPrinted);
            cell->setAttributes(m_lastCharAttr);
            markCellDirty(row, col);
        }

        ++col;
    }

    m_cursor->set(row, col); // advance cursor
    requestRender();
}

void Screen::setLineDisplayMode(int row, ansi::LineDisplayMode mode)
{
    if (row < 0 || row >= m_rows)
        return;

    auto &buffer = activeBuffer();
    for (int col = 0; col < m_columns; ++col) {
        auto &cell = buffer[index(row, col)];
        if (!cell.isProtectedForWrite()) {
            cell.setLineDisplayMode(mode);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


LineDisplayMode Screen::getLineDisplayMode(int row) const
{
    if (row < 0 || row >= m_rows)
        return LineDisplayMode::SingleWidth;

    return activeBuffer()[index(row, 0)].getLineDisplayMode();
}

void Screen::setLineWidth(int row, int width)
{
    if (row < 0 || row >= m_rows)
        return;

    auto &buffer = activeBuffer();
    for (int col = 0; col < m_columns; ++col) {
        auto &cell = buffer[index(row, col)];
        if (!cell.isProtectedForWrite()) {
            cell.setLineWidth(width);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


void Screen::setLineHeight(int row, int height)
{
    if (row < 0 || row >= m_rows)
        return;

    auto &buffer = activeBuffer();
    for (int col = 0; col < m_columns; ++col) {
        auto &cell = buffer[index(row, col)];
        if (!cell.isProtectedForWrite()) {
            cell.setLineHeight(height);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


void Screen::setLineHeightPosition(int row, bool isTop)
{
    if (row < 0 || row >= m_rows)
        return;

    auto &buffer = activeBuffer();
    for (int col = 0; col < m_columns; ++col) {
        auto &cell = buffer[index(row, col)];
        if (!cell.isProtectedForWrite()) {
            cell.setLineHeightPosition(isTop);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


bool Screen::originMode() const
{
    return m_originMode;
}

// Clear the entire active buffer but leave scrollback intact
void Screen::clear()
{
    std::cerr << "[Screen::clear] Clearing with current attributes\n";
    auto &buffer = activeBuffer();
    // const auto attrs = m_currentAttributes;  // ← capture current attrs

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            Cell &cell = buffer[index(row, col)];

            if (!cell.isProtectedForErase()) {
                cell = Cell(U' ', m_currentAttributes);   // ← preserve active color/styles
                markCellDirty(row, col);
            }
        }
    }

    setCursor(0, 0);
    m_dirtyRows.clear();
    m_dirtyCols.clear();
    requestRender();
}

// Reset the whole terminal state (used by ESC c and others)
void Screen::reset()
{
    clear();
    clearScrollback();
    resetTabStops();
    m_inAlternateBuffer = false;
    m_originMode = false;
    m_scrollTop    = 0;
    m_scrollBottom = m_rows;
    m_currentAttributes = Cell::defaultAttributes();
    m_tabStops.reset();
    // Set default tab stops every 8 columns on reset, up to m_columns - 1
    for (int i = 0; i < m_columns - 1; i += 8)
        setTabStop(i);

    std::cerr << "[Screen::reset] Default tab stops set every 8 columns up to " << m_columns - 1 << std::endl;
    requestRender();
}

// Clear only the line that the cursor is currently on
void Screen::clearLine()
{
    clearRow(m_cursor->row());
}

// Fly-weight accessor used by SGR dispatcher
Attributes *Screen::currentAttributes()
{
    return m_currentAttributes.get();
}

Cell *Screen::currentCell()
{
    return cellAt(m_cursor->row(), m_cursor->col());
}

// Simple accessors used by many CSI handlers
int Screen::scrollTop() const
{
    return m_scrollTop;
}
int Screen::scrollBottom() const
{
    return m_scrollBottom;
}


}

