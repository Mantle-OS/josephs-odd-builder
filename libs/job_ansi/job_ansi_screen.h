#pragma once

#include <memory>
#include <unordered_set>
#include <vector>
#include <functional>
#include <deque>
#include <bitset>
#include <unordered_map>
#include <optional>

#include "job_ansi_attributes.h"
#include "job_ansi_cell.h"
#include "job_ansi_cursor.h"

#include "utils/job_ansi_utils.h"
#include "utils/calback_macros.h"

#include "utils/charset_translator.h"

namespace job::ansi {

using job::ansi::utils::RGBColor;

constexpr std::size_t MAX_SCROLLBACK_LINES = 10000;
static_assert(MAX_SCROLLBACK_LINES > 0, "MAX_SCROLLBACK_LINES must be greater than 0");

class Screen {
    // API Functional callbacks
    API_CALLBACK(bool                               , wraparound                , true              )
    API_CALLBACK(bool                               , insertMode                , false             )
    API_CALLBACK(bool                               , autoLinefeed              , false             )
    API_CALLBACK(bool                               , bracketedPasteEnabled     , false             )

    API_CALLBACK_CONST(std::string                  , windowTitle               , ""                ) // OSC(API)
    API_CALLBACK_CONST(std::string                  , windowIconTitle           , ""                ) // OSC(API)
    API_CALLBACK_CONST(std::string                  , window_reply              , ""                ) // CSI ... t replies
    API_CALLBACK_CONST(std::string                  , hyperlinkAnchor           , ""                ) // OSC(API)
    API_CALLBACK_CONST(std::string                  , clipboardData             , ""                ) // OSC(API)
    API_CALLBACK_CONST(std::string                  , device_attr               , "\033[?1;0c"      ) // CSI[DA]
    API_CALLBACK_CONST(std::string                  , device_attr_secondary     , ""                ) // CSI[DA2]
    API_CALLBACK_CONST(std::string                  , device_attr_tertiary      , ""                ) // CSI[DA3]

    API_CALLBACK(bool                               , blinkingCursor            , true              ) //
    API_CALLBACK(bool                               , cursorVisible             , true              )
    API_CALLBACK_CONST(std::string                  , cursor_report             , ""                ) // CSI[6n]
    API_CALLBACK_CONST(std::string                  , cursor_position_report    , ""                ) // CSI[CPR]
    API_CALLBACK(std::optional<ansi::utils::RGBColor>            , cursorColor               , std::nullopt      )

    API_CALLBACK(bool                               , appKeypadMode             , false             ) // ESC_CODE: enable disable _APP_KEYPAP
    API_CALLBACK_CONST(std::string                  , terminal_ident_reply      , "\x1B[?1;2c"      ) // ESC IDENTIFY_TERMINAL (DECID)
    API_CALLBACK(bool                               , cursorKeysMode            , false             ) // DECSET: ?1h / ?1l
    API_CALLBACK(bool                               , reverseVideo              , false             ) // DECSCNM

    // MOUSE
    API_CALLBACK(bool, mouseX10Mode         , false)
    API_CALLBACK(bool, mouseVT200Highlight  , false)
    API_CALLBACK(bool, mouseButtonEventMode , false)
    API_CALLBACK(bool, mouseAnyEventMode    , false)
    API_CALLBACK(bool, mouseUtf8Encoding    , false)
    API_CALLBACK(bool, mouseSgrEncoding     , false)

public:
    explicit Screen(int rows = 24, int cols = 80);

    std::function<void(const std::string &)> on_terminalOutput;
    void sendString(const std::string &str)
    {
        if(on_terminalOutput)
            on_terminalOutput(str);
    }

    std::function<void()> renderCallback;
    void requestRender()
    {
        m_needsRender = true;
        if (renderCallback)
            renderCallback();
    }

    // BUFFER UTILITIES
    inline int index(int row, int col) const
    {
        return row * m_columns + col;
    }

    // ROW UTILITIES
    int rowCount() const
    {
        return m_rows;
    }
    void clearLine();
    void insertLines(int startRow, int count);
    void deleteLines(int count);
    void deleteLines(int startRow, int count);
    void clearRow(int row);
    void copyRow(int from, int to);

    // COLUMN UTILITIES
    int columnCount() const {return m_columns;}

    // Painting utils
    void markCellDirty(int row, int col);
    void clearDirty();
    const std::unordered_set<int> &dirtyRows() const;
    const std::unordered_set<int> &dirtyCols(int row) const;

    // BUFFER OPERATIONS
    void resize(int rows, int cols);
    void clear();
    void reset();

    // CHARACTER OPERATIONS
    void putChar(char32_t ch);
    void putChar(char32_t ch, Attributes *style);

    // ACTIVE BUFFER
    std::vector<Cell> &activeBuffer();
    const std::vector<Cell> &activeBuffer() const;
    void enterAlternateBuffer();
    void exitAlternateBuffer();
    bool isAlternateBufferActive() const;

    // CELL ACCESS
    Cell *cellAt(int row, int col);
    Attributes *currentAttributes();
    void setCurrentAttributes(const std::shared_ptr<Attributes> &attr)
    {
        m_currentAttributes = attr;
    }
    Cell *currentCell();

    // SCROLLBACK

    void setScrollRegion(int top, int bottom);
    int  scrollTop() const;
    int  scrollBottom() const;
    void scrollUp(int top, int bottom, int lines = 1);
    void scrollUp();
    void scrollDown(int top, int bottom, int lines = 1);
    // void scrollDown();
    const std::deque<std::vector<Cell>> &scrollbackLines() const;
    void clearScrollback();
    void scrollbackPageUp();
    void scrollbackPageDown();
    int  scrollOffset() const;
    void setScrollOffset(int offset);
    void scrollbackLineUp();
    void scrollbackLineDown();
    bool isInScrollback() const;

    // CURSOR OPERATIONS
    const Cursor *cursor() const
    {
        return m_cursor.get();
    }

    Cursor *cursor()
    {
        return m_cursor.get();
    }
    void moveCursor(int rowDelta, int colDelta);
    void moveCursorRelative(int rowDelta, int colDelta, int regionTop, int regionBottom);
    void moveCursorRelative(int rowDelta, int colDelta)
    {
        moveCursorRelative(rowDelta, colDelta, originRow(), originBottom());
    }
    void setCursor(int row, int col);
    void saveCursor();
    void restoreCursor();
    void setCursorStyle(int style);

    // MOUSE
    std::function <void(const std::string & name )> on_mouse_info;
    void sendMouseSGR(int button, bool pressed, int row, int col);
    void sendMouseX10(int button, bool /*pressed*/, int row, int col);
    void sendMouseVT200Highlight(int button, bool pressed, int row, int col);
    void sendMouseButtonEventMode(int button, bool pressed, int row, int col);
    void sendMouseAnyEventMode(int button, bool pressed, int row, int col);
    void sendMouseUtf8Encoding(int button, bool pressed, int row, int col);

    // FOCUS
    void set_focusEventEnabled(bool enable)
    {
        m_focusEventEnabled = enable;
    }
    bool get_focusEventEnabled() const
    {
        return m_focusEventEnabled;
    }
    void sendFocusEvent(bool focused);

    // ORIGIN MODE
    bool originMode() const;
    void set_originMode(bool enabled);
    int originRow() const;
    int originBottom() const;

    // TAB STOPS
    void setTabStop(int col);
    void clearTabStop(int col);
    void clearAllTabStops();
    bool isTabStop(int col) const;
    void resetTabStops()
    {
        m_tabStops.reset();  // Clear all tab stops
        for (int col = 0; col < m_columns; col += 8)
            m_tabStops.set(col);
    }

    // CHARSET
    void selectCharset(int set, CharsetDesignator designator);
    char32_t applyCharset(char32_t ch) const;
    CharsetDesignator charsetForCell(int row, int col) const;
    CharsetDesignator g0Charset() const;  // remove force resolveCharsetChar
    CharsetDesignator g1Charset() const; // remove force resolveCharsetChar
    int activeCharset() const;
    char32_t resolveCharsetChar(char32_t ch) const;
    char32_t mapCharForCharset(char32_t ch, CharsetDesignator charset) const;

    // Erase operations
    void eraseToEnd(int row, int col);
    void eraseToStart(int row, int col);
    void eraseScreen();
    void eraseScreenAndScrollback();
    void eraseLineToEnd(int row, int col);
    void eraseLineToStart(int row, int col);
    void eraseLine(int row);
    void deleteChars(int count);
    void eraseChars(int count);
    void repeatLastChar(int count);
    void eraseField(int row, int col, EF_MODE mode)
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_columns)
            return;

        switch (mode) {
        case EF_MODE::TO_END:
            for (int c = col; c < m_columns; ++c)
                if (!cellAt(row, c)->isProtectedForErase())
                    cellAt(row, c)->reset();
            break;
        case EF_MODE::TO_START:
            for (int c = 0; c <= col; ++c)
                if (!cellAt(row, c)->isProtectedForErase())
                    cellAt(row, c)->reset();
            break;
        case EF_MODE::ENTIRE_LINE:
            for (int c = 0; c < m_columns; ++c)
                if (!cellAt(row, c)->isProtectedForErase())
                    cellAt(row, c)->reset();
            break;
        default:
            break;
        }

        requestRender();
    }
    void eraseArea(EA_MODE mode)
    {
        if (!m_cursor)
            return;

        int curRow = m_cursor->row();
        int curCol = m_cursor->col();

        switch (mode) {
        case EA_MODE::TO_END:
            for (int r = curRow; r < m_rows; ++r) {
                int startCol = (r == curRow ? curCol : 0);
                for (int c = startCol; c < m_columns; ++c) {
                    Cell *cell = cellAt(r, c);
                    if (cell && !cell->isProtectedForErase())
                        cell->reset();
                }
            }
            break;
        case EA_MODE::TO_START:
            for (int r = 0; r <= curRow; ++r) {
                int endCol = (r == curRow ? curCol : m_columns - 1);
                for (int c = 0; c <= endCol; ++c) {
                    Cell *cell = cellAt(r, c);
                    if (cell && !cell->isProtectedForErase())
                        cell->reset();
                }
            }
            break;
        case EA_MODE::ENTIRE_SCREEN:
            for (int r = 0; r < m_rows; ++r) {
                for (int c = 0; c < m_columns; ++c) {
                    Cell *cell = cellAt(r, c);
                    if (cell && !cell->isProtectedForErase())
                        cell->reset();
                }
            }
            break;
        }

        requestRender();
    }

    void pushWindowTitle()
    {
        m_titleStack.push_back(m_windowTitle);
    }

    void popWindowTitle()
    {
        if (!m_titleStack.empty()) {
            m_windowTitle = m_titleStack.back();
            m_titleStack.pop_back();
            requestRender(); // if needed
        }
    }

    // Line width/height operations
    void setLineDisplayMode(int row, LineDisplayMode mode);
    LineDisplayMode getLineDisplayMode(int row) const;
    void setLineWidth(int row, int width);
    void setLineHeight(int row, int height);
    void setLineHeightPosition(int row, bool isTop);

    // SEE (Select Editing Extent)
    void setEditingExtent(SEE_MODE mode)
    {
        m_editingExtent = mode;
    }
    SEE_MODE editingExtent() const
    {
        return m_editingExtent;
    }

    void setProtectionMode(DECSCA_MODE mode)
    {
        m_protectionMode = mode;
    }
    DECSCA_MODE protectionMode() const
    {
        return m_protectionMode;
    }


private:
    struct SavedBufferState {
        std::unique_ptr<Cursor> cursor;
        int scrollTop;
        int scrollBottom;
        std::bitset<512> tabStops;
    };

    bool                                                m_needsRender = false;
    std::unordered_set<int>                             m_dirtyRows;
    std::unordered_map<int, std::unordered_set<int>>    m_dirtyCols;

    int                                                 m_rows = 24;
    int                                                 m_columns = 80;
    std::vector<Cell>                                   m_mainBuffer;
    std::vector<Cell>                                   m_altBuffer;
    std::deque<std::vector<Cell>>                       m_scrollback;

    std::unique_ptr<Cursor>                             m_cursor;
    std::unique_ptr<Cursor>                             m_savedCursor;

    Attributes::Ptr                                     m_currentAttributes = Cell::defaultAttributes();

    utils::CharsetTranslator                            m_charset;

    bool                                                m_inAlternateBuffer = false;
    bool                                                m_originMode = false;
    int                                                 m_scrollTop = 0;
    int                                                 m_scrollBottom = m_rows;
    int                                                 m_scrollOffset = 0;
    std::bitset<512>                                    m_tabStops;

    std::optional<SavedBufferState>                     m_savedState;
    bool                                                m_focusEventEnabled = false;
    char32_t                                            m_lastCharPrinted = U' ';
    Attributes                                          m_lastCharAttr;
    SEE_MODE                                            m_editingExtent = SEE_MODE::LINE;
    DECSCA_MODE                                         m_protectionMode = DECSCA_MODE::NONE;
    std::vector<std::string>                            m_titleStack;

};

}
