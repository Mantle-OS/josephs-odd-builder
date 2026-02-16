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
#include "job_ansi_grid.h"
#include "job_ansi_scrollback.h"
#include "job_ansi_saved_buffer_state.h"
#include "job_ansi_line_display_mode.h"

#include "utils/job_ansi_utils.h"
#include "utils/calback_macros.h"


#include "utils/charset_translator.h"

namespace job::ansi {

using job::ansi::utils::RGBColor;

constexpr std::size_t MAX_SCROLLBACK_LINES = 10000;
static_assert(MAX_SCROLLBACK_LINES > 0, "MAX_SCROLLBACK_LINES must be greater than 0");

class Screen {
    // API Functional callbacks
    JOB_ANSI_SCREEN_CALLBACK(bool                               , wraparound                , true              )
    JOB_ANSI_SCREEN_CALLBACK(bool                               , insertMode                , false             )
    JOB_ANSI_SCREEN_CALLBACK(bool                               , autoLinefeed              , false             )
    JOB_ANSI_SCREEN_CALLBACK(bool                               , bracketedPasteEnabled     , false             )

    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , windowTitle               , ""                ) // OSC(API)
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , windowIconTitle           , ""                ) // OSC(API)
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , window_reply              , ""                ) // CSI ... t replies
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , hyperlinkAnchor           , ""                ) // OSC(API)
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , clipboardData             , ""                ) // OSC(API)
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , device_attr               , "\033[?1;0c"      ) // CSI[DA]
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , device_attr_secondary     , ""                ) // CSI[DA2]
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , device_attr_tertiary      , ""                ) // CSI[DA3]

    JOB_ANSI_SCREEN_CALLBACK(bool                               , blinkingCursor            , true              ) //
    JOB_ANSI_SCREEN_CALLBACK(bool                               , cursorVisible             , true              )
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , cursor_report             , ""                ) // CSI[6n]
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , cursor_position_report    , ""                ) // CSI[CPR]
    JOB_ANSI_SCREEN_CALLBACK(std::optional<ansi::utils::RGBColor>            , cursorColor               , std::nullopt      )

    JOB_ANSI_SCREEN_CALLBACK(bool                               , appKeypadMode             , false             ) // ESC_CODE: enable disable _APP_KEYPAP
    JOB_ANSI_SCREEN_CALLBACK_CONST(std::string                  , terminal_ident_reply      , "\x1B[?1;2c"      ) // ESC IDENTIFY_TERMINAL (DECID)
    JOB_ANSI_SCREEN_CALLBACK(bool                               , cursorKeysMode            , false             ) // DECSET: ?1h / ?1l
    JOB_ANSI_SCREEN_CALLBACK(bool                               , reverseVideo              , false             ) // DECSCNM

    // MOUSE
    JOB_ANSI_SCREEN_CALLBACK(bool, mouseX10Mode         , false)
    JOB_ANSI_SCREEN_CALLBACK(bool, mouseVT200Highlight  , false)
    JOB_ANSI_SCREEN_CALLBACK(bool, mouseButtonEventMode , false)
    JOB_ANSI_SCREEN_CALLBACK(bool, mouseAnyEventMode    , false)
    JOB_ANSI_SCREEN_CALLBACK(bool, mouseUtf8Encoding    , false)
    JOB_ANSI_SCREEN_CALLBACK(bool, mouseSgrEncoding     , false)

public:
    explicit Screen(int rows = 24, int cols = 80);

    std::function<void(const std::string &)> on_terminalOutput;
    void sendString(const std::string &str)
    {
        if(on_terminalOutput)
            on_terminalOutput(str);
    }

    std::function<void()> renderCallback;
    void requestRender();

    // ROW UTILITIES
    int rowCount() const;
    void clearLine();
    void insertLines(int startRow, int count);
    void deleteLines(int count);
    void deleteLines(int startRow, int count);
    void clearRow(int row);
    void copyRow(int from, int to);

    // COLUMN UTILITIES
    int columnCount() const
    {
        return m_primaryGrid.columns();
    }

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
    Grid &activeGrid();
    const Grid &activeGrid() const;
    void enterAlternateGrid();
    void exitAlternateGrid();
    bool isAlternateGridActive() const;

    // CELL ACCESS
    Cell *cellAt(int row, int col);
    Attributes *currentAttributes();
    void setCurrentAttributes(const Attributes::Ptr &attr);
    Cell *currentCell();

    // SCROLLBACK
    void setScrollRegion(int top, int bottom);
    int  scrollTop() const;
    int  scrollBottom() const;
    void scrollUp(int top, int bottom, int lines = 1);
    void scrollUp();
    void scrollDown(int top, int bottom, int lines = 1);
    // void scrollDown();

    // SCROLLBACK HISTORY
    [[nodiscard]] const ScrollbackBuffer &scrollback() const;
    void clearScrollback();
    void scrollbackPageUp();
    void scrollbackPageDown();
    void scrollbackLineUp();
    void scrollbackLineDown();
    int  scrollOffset() const;
    void setScrollOffset(int offset);
    bool isInScrollback() const;

    // CURSOR OPERATIONS
    const Cursor *cursor() const;
    Cursor *cursor();
    void moveCursor(int rowDelta, int colDelta);
    void moveCursorRelative(int rowDelta, int colDelta, int regionTop, int regionBottom);
    void moveCursorRelative(int rowDelta, int colDelta);
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
    void set_focusEventEnabled(bool enable);
    bool get_focusEventEnabled() const;
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
    void resetTabStops();

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
    void eraseField(int row, int col, EF_MODE mode);
    void eraseArea(EA_MODE mode);

    void pushWindowTitle();
    void popWindowTitle();

    // Line width/height operations
    void setLineDisplayMode(int row, LineDisplayMode mode);
    LineDisplayMode getLineDisplayMode(int row) const;
    void setLineWidth(int row, int width);
    void setLineHeight(int row, int height);
    void setLineHeightPosition(int row, bool isTop);

    // SEE (Select Editing Extent)
    void setEditingExtent(SEE_MODE mode);
    SEE_MODE editingExtent() const;

    void setProtectionMode(DECSCA_MODE mode);
    DECSCA_MODE protectionMode() const;


private:
    bool                                                m_needsRender = false;
    std::unordered_set<int>                             m_dirtyRows;
    std::unordered_map<int, std::unordered_set<int>>    m_dirtyCols;

    Grid                                                m_primaryGrid;
    Grid                                                m_altGrid;

    ScrollbackBuffer                                    m_scrollback;
    int                                                 m_scrollOffset{0};

    Cursor::UPtr                                        m_cursor;
    Cursor::UPtr                                        m_savedCursor;

    Attributes::Ptr                                     m_currentAttributes = Cell::defaultAttributes();

    utils::CharsetTranslator                            m_charset;

    bool                                                m_inAlternateGrid = false;
    bool                                                m_originMode = false;
    bool                                                m_wrapPending = false;
    int                                                 m_scrollTop = 0;
    int                                                 m_scrollBottom = 24; // matches up to Grid::m_rows
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
