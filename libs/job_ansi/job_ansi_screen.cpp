#include "job_ansi_screen.h"
#include <algorithm>
#include <memory>
#include <cassert>

namespace job::ansi {

const Cell kDefaultCell{};
Screen::Screen(int rows, int cols):
    m_primaryGrid(rows, cols),
    m_altGrid(rows, cols),
    m_scrollback(MAX_SCROLLBACK_LINES),
    m_cursor(std::make_unique<Cursor>(rows, cols)),
    m_savedCursor(std::make_unique<Cursor>(rows, cols)),
    m_currentAttributes(Attributes::intern(Attributes())),
    m_scrollBottom(rows)
{
    resize(rows, cols);

    // Set default tab stops every 8 columns, up to m_columns - 1
    for (int col = 0; col < m_primaryGrid.columns(); col += 8)
        setTabStop(col);
}

void Screen::requestRender()
{
    m_needsRender = true;
    if (renderCallback)
        renderCallback();
}

int Screen::rowCount() const
{
    return m_primaryGrid.rows();
}

void Screen::insertLines(int startRow, int count)
{
    int limit = m_scrollBottom;
    activeGrid().insertLines(startRow, count, Cell::blank(), limit);

    for(int r = startRow; r < limit; ++r)
        m_dirtyRows.insert(r);

    requestRender();
}

void Screen::deleteLines(int count)
{
    deleteLines(m_cursor->row(), count);
}

void Screen::deleteLines(int startRow, int count)
{
    int limit = m_scrollBottom;
    if (startRow < m_scrollTop || startRow >= limit)
        return;

    activeGrid().deleteLines(startRow, count, Cell::blank(), limit);
    for(int r = startRow; r < limit; ++r)
        m_dirtyRows.insert(r);

    requestRender();
}

void Screen::clearRow(int row)
{
    auto &grid = activeGrid();
    if (row < 0 || row >= grid.rows())
        return;

    // Hoist the row span for speed
    auto rowSpan = grid[row];

    for (int col = 0; col < grid.columns(); ++col) {
        auto& cell = rowSpan[col];
        if (!cell.isProtectedForErase()) {
            cell.reset();
            markCellDirty(row, col);
        }
    }
    requestRender();
}

void Screen::copyRow(int from, int to)
{
    auto &grid = activeGrid();
    if (from < 0 || from >= grid.rows() || to < 0 || to >= grid.rows())
        return;

    auto srcSpan = grid[from];
    auto dstSpan = grid[to];

    for (int col = 0; col < grid.columns(); ++col) {
        if (!dstSpan[col].isProtectedForWrite()) {
            dstSpan[col] = srcSpan[col];
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
    m_primaryGrid.resize(rows, cols);
    m_altGrid.resize(rows, cols);

    if (m_cursor)
        m_cursor->updateDimensions(rows, cols);

    if (m_savedCursor)
        m_savedCursor->updateDimensions(rows, cols);

    m_tabStops.reset();
    for (int col = 0; col < cols; col += 8)
        setTabStop(col);

    if (m_scrollBottom == m_primaryGrid.rows() || m_scrollBottom > rows)
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
        m_wrapPending = false;
        moveCursor(m_cursor->row(), 0);
        if (get_autoLinefeed()) {
            if (m_cursor->row() + 1 >= m_scrollBottom)
                scrollUp();
            moveCursor(std::min(m_cursor->row() + 1, m_scrollBottom - 1), 0);
        }
        return;
    } else if (ch == U'\n') {
        m_wrapPending = false;
        if (m_cursor->row() + 1 >= m_scrollBottom)
            scrollUp();

        moveCursor(std::min(m_cursor->row() + 1, m_scrollBottom - 1), m_cursor->column());
        return;
    } else if (ch == U'\b') {
        m_wrapPending = false;
        if (m_cursor->column() > 0)
            moveCursor(m_cursor->row(), m_cursor->column() - 1);
        return;
    }

    if (m_wrapPending && get_wraparound()) {
        m_wrapPending = false;

        // Perform the wrapping logic here
        int nextRow = m_cursor->row() + 1;
        if (nextRow >= m_scrollBottom) {
            scrollUp();
            nextRow = m_scrollBottom - 1;
        }
        // Wrap to start of next line
        m_cursor->setPosition(nextRow, 0);
    }

    auto &grid = activeGrid();
    const int g_rows = grid.rows();
    const int g_cols = grid.columns();

    // Skip out-of-bounds writes
    if (m_cursor->row() < 0 || m_cursor->row() >= g_rows ||
        m_cursor->column() < 0 || m_cursor->column() >= g_cols)
        return;

    ch = applyCharset(ch);


    const int row = m_cursor->row();
    const int col = m_cursor->column();
    // Insert Mode: shift characters right before writing

    // Insert Mode
    if (get_insertMode()) {
        for (int i = g_cols - 2; i >= col; --i) {
            grid[row, i + 1] = grid[row, i];
            markCellDirty(row, i + 1);
        }
        grid[row, col].reset();
    }
    auto &cell = grid[row, col];

    // Respect write protection
    if (!cell.isProtectedForWrite()){
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
        bool atRightEdge = (col == g_cols - 1);

        if (atRightEdge) {
            if (get_wraparound()) {
                // DO NOT Scroll yet. Just set the flag.
                m_wrapPending = true;
                // Cursor stays visibly at the last column
            } else {
                // No wraparound: cursor stays clamped at edge
            }
        } else {
            // Normal advance
            m_cursor->move(0, 1);
        }

        requestRender();
    }
}


Grid &Screen::activeGrid()
{
    return m_inAlternateGrid ? m_altGrid : m_primaryGrid;
}

const Grid &Screen::activeGrid() const
{
    return m_inAlternateGrid ? m_altGrid : m_primaryGrid;
}

void Screen::enterAlternateGrid()
{
    m_inAlternateGrid = true;
    std::fill(m_altGrid.begin(), m_altGrid.end(), Cell{});
    setScrollRegion(0, m_altGrid.rows());  // Reset to full screen
    requestRender();
}

void Screen::exitAlternateGrid()
{
    m_inAlternateGrid = false;
    requestRender();
}

bool Screen::isAlternateGridActive() const
{
    return m_inAlternateGrid;
}

Cell *Screen::cellAt(int row, int col)
{
    auto &grid = activeGrid();
    if (row < 0 || row >= grid.rows() || col < 0 || col >= grid.columns())
        return nullptr;
    return &grid[row, col];
}

void Screen::setScrollRegion(int top, int bottom)
{
    m_scrollTop = std::clamp(top, 0, activeGrid().rows() - 1);
    m_scrollBottom = std::clamp(bottom, m_scrollTop + 1, activeGrid().rows());
}

void Screen::scrollUp(int top, int bottom, int lines)
{
    if (lines <= 0 || top >= bottom)
        return;

    auto &grid = activeGrid();
    if (top == 0 && !isAlternateGridActive()) {
        for (int i = 0; i < lines; ++i) {
            auto rowSpan = grid[top + i];
            Cell::Line savedLine(rowSpan.begin(), rowSpan.end());
            if (m_scrollback.push_back(std::move(savedLine)))
                if (m_scrollOffset > 0) m_scrollOffset--;
        }
    }

    grid.deleteLines(top, lines, Cell::blank(), bottom);

    if (m_cursor->row() >= bottom)
        m_cursor->setPosition(bottom - 1, 0);
    else if (m_cursor->row() < top)
        m_cursor->setPosition(top, 0);


    for(int r = top; r < bottom; ++r)
        m_dirtyRows.insert(r);

    requestRender();
}

void Screen::scrollUp()
{
    scrollUp(m_scrollTop, m_scrollBottom, 1);
}

void Screen::scrollDown(int top, int bottom, int lines)
{
    if (lines <= 0 || top >= bottom)
        return;

    auto &grid = activeGrid();
    grid.insertLines(top, lines, Cell::blank(), bottom);

    for(int r = top; r < bottom; ++r)
        m_dirtyRows.insert(r);
    requestRender();
}
const ScrollbackBuffer &Screen::scrollback() const
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
        m_scrollOffset = std::min<int>(m_scrollOffset + activeGrid().rows(), m_scrollback.size());
        requestRender();
    }
}

void Screen::scrollbackPageDown()
{
    if (m_scrollOffset > 0) {
        m_scrollOffset = std::max(0, m_scrollOffset - activeGrid().rows());
        requestRender();
    }
}

int Screen::scrollOffset() const
{
    if (m_scrollOffset < 0)
        return 0;

    if (m_scrollOffset > (int)m_scrollback.size())
        return (int)m_scrollback.size();

    return m_scrollOffset;
}

void Screen::setScrollOffset(int offset)
{
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

const Cursor *Screen::cursor() const
{
    return m_cursor.get();
}

Cursor *Screen::cursor()
{
    return m_cursor.get();
}

void Screen::moveCursor(int rowDelta, int colDelta)
{
    // std::cerr << "[Cursor] moveCursor(" << rowDelta << ", " << colDelta << ")\n";

    int regionTop = originRow();
    int regionBottom = originBottom();
    int newRow = std::clamp(m_cursor->row() + rowDelta, regionTop, regionBottom - 1);
    int newCol = std::clamp(m_cursor->column() + colDelta, 0, columnCount() - 1);
    setCursor(newRow, newCol); // no longer subtract regionTop
}
void Screen::moveCursorRelative(int rowDelta, int colDelta, int regionTop, int regionBottom)
{
    if (!m_cursor)
        return;

    int baseRow  = regionTop;
    int limitRow = regionBottom - 1;

    int newRow = std::clamp(m_cursor->row() + rowDelta, baseRow, limitRow);
    int newCol = std::clamp(m_cursor->column() + colDelta, 0, columnCount() - 1);

    setCursor(newRow, newCol);  // absolute
}

void Screen::moveCursorRelative(int rowDelta, int colDelta)
{
    moveCursorRelative(rowDelta, colDelta, originRow(), originBottom());
}


void Screen::setCursor(int row, int col)
{
    m_wrapPending = false; // Explicit movement resets wrap state
    // std::cerr << "[Cursor] setCursor(" << row << ", " << col << ")\n";

    int clampedRow = std::clamp(row, 0, rowCount() - 1);
    int clampedCol = std::clamp(col, 0, columnCount() - 1);
    if (m_cursor)
        m_cursor->setPosition(clampedRow, clampedCol);
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

void Screen::set_focusEventEnabled(bool enable)
{
    m_focusEventEnabled = enable;
}

bool Screen::get_focusEventEnabled() const
{
    return m_focusEventEnabled;
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
    return m_originMode ? m_scrollBottom : activeGrid().rows();
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

void Screen::resetTabStops()
{
    m_tabStops.reset();  // Clear all tab stops
    for (int col = 0; col < activeGrid().columns(); col += 8)
        m_tabStops.set(col);
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
    int col = m_cursor->column();
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
    int col = m_cursor->column();

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
    int col = m_cursor->column();

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

    m_cursor->setPosition(row, col); // advance cursor
    requestRender();
}

void Screen::eraseField(int row, int col, EF_MODE mode)
{
    if (row < 0 || row >= activeGrid().rows() || col < 0 || col >= activeGrid().columns())
        return;

    switch (mode) {
    case EF_MODE::TO_END:
        for (int c = col; c < activeGrid().columns(); ++c)
            if (!cellAt(row, c)->isProtectedForErase())
                cellAt(row, c)->reset();
        break;
    case EF_MODE::TO_START:
        for (int c = 0; c <= col; ++c)
            if (!cellAt(row, c)->isProtectedForErase())
                cellAt(row, c)->reset();
        break;
    case EF_MODE::ENTIRE_LINE:
        for (int c = 0; c < activeGrid().columns(); ++c)
            if (!cellAt(row, c)->isProtectedForErase())
                cellAt(row, c)->reset();
        break;
    default:
        break;
    }

    requestRender();
}

void Screen::eraseArea(EA_MODE mode)
{
    if (!m_cursor)
        return;

    const int rows = activeGrid().rows();
    const int cols = activeGrid().columns();

    const int curRow = m_cursor->row();
    const int curCol = m_cursor->column();


    switch (mode) {
    case EA_MODE::TO_END:
        for (int r = curRow; r < rows; ++r) {
            int startCol = (r == curRow ? curCol : 0);
            for (int c = startCol; c < cols; ++c) {
                Cell *cell = cellAt(r, c);
                if (cell && !cell->isProtectedForErase())
                    cell->reset();
            }
        }
        break;
    case EA_MODE::TO_START:
        for (int r = 0; r <= curRow; ++r) {
            int endCol = (r == curRow ? curCol : cols - 1);
            for (int c = 0; c <= endCol; ++c) {
                Cell *cell = cellAt(r, c);
                if (cell && !cell->isProtectedForErase())
                    cell->reset();
            }
        }
        break;
    case EA_MODE::ENTIRE_SCREEN:
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                Cell *cell = cellAt(r, c);
                if (cell && !cell->isProtectedForErase())
                    cell->reset();
            }
        }
        break;
    }

    requestRender();
}

void Screen::pushWindowTitle()
{
    m_titleStack.push_back(m_windowTitle);
}

void Screen::popWindowTitle()
{
    if (!m_titleStack.empty()) {
        m_windowTitle = m_titleStack.back();
        m_titleStack.pop_back();
        requestRender(); // if needed
    }
}

void Screen::setLineDisplayMode(int row, ansi::LineDisplayMode mode)
{

    if (row < 0 || row >= activeGrid().rows())
        return;

    auto &buffer = activeGrid();
    for (int col = 0; col < activeGrid().columns(); ++col) {
        auto &cell = buffer[row, col];
        if (!cell.isProtectedForWrite()) {
            cell.setLineDisplayMode(mode);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


LineDisplayMode Screen::getLineDisplayMode(int row) const
{
    if (row < 0 || row >= activeGrid().rows())
        return LineDisplayMode::SingleWidth;

    return activeGrid()[row, 0].getLineDisplayMode();
}

void Screen::setLineWidth(int row, int width)
{
    if (row < 0 || row >= activeGrid().rows())
        return;

    auto &buffer = activeGrid();
    for (int col = 0; col < buffer.columns(); ++col) {
        auto &cell = buffer[row, col];
        if (!cell.isProtectedForWrite()) {
            cell.setLineWidth(width);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


void Screen::setLineHeight(int row, int height)
{
    auto &buffer = activeGrid();
    if (row < 0 || row >= buffer.rows())
        return;

    for (int col = 0; col < buffer.columns(); ++col) {
        auto &cell = buffer[row, col];
        if (!cell.isProtectedForWrite()) {
            cell.setLineHeight(height);
            markCellDirty(row, col);
        }
    }
    requestRender();
}


void Screen::setLineHeightPosition(int row, bool isTop)
{
    auto &buffer = activeGrid();
    if (row < 0 || row >= buffer.rows())
        return;

    for (int col = 0; col < buffer.columns(); ++col) {
        auto &cell = buffer[row, col];
        if (!cell.isProtectedForWrite()) {
            cell.setLineHeightPosition(isTop);
            markCellDirty(row, col);
        }
    }
    requestRender();
}

void Screen::setEditingExtent(SEE_MODE mode)
{
    m_editingExtent = mode;
}

SEE_MODE Screen::editingExtent() const
{
    return m_editingExtent;
}

void Screen::setProtectionMode(DECSCA_MODE mode)
{
    m_protectionMode = mode;
}

DECSCA_MODE Screen::protectionMode() const
{
    return m_protectionMode;
}


bool Screen::originMode() const
{
    return m_originMode;
}

// Clear the entire active buffer but leave scrollback intact
void Screen::clear()
{;
    auto &buffer = activeGrid();
    // const auto attrs = m_currentAttributes;  //<- capture current attrs

    for (int row = 0; row < buffer.rows(); ++row) {
        for (int col = 0; col < buffer.columns(); ++col) {
            Cell &cell = buffer[row, col];

            if (!cell.isProtectedForErase()) {
                cell = Cell(U' ', m_currentAttributes);   //<- preserve active color/styles
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
    m_inAlternateGrid = false;
    m_originMode = false;
    m_scrollTop    = 0;
    m_scrollBottom = activeGrid().rows();
    m_currentAttributes = Cell::defaultAttributes();
    m_tabStops.reset();
    // Set default tab stops every 8 columns on reset, up to m_columns - 1
    for (int i = 0; i < activeGrid().columns() - 1; i += 8)
        setTabStop(i);

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

void Screen::setCurrentAttributes(const std::shared_ptr<Attributes> &attr)
{
    m_currentAttributes = attr;
}

Cell *Screen::currentCell()
{
    return cellAt(m_cursor->row(), m_cursor->column());
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

} // namespace job::ansi
