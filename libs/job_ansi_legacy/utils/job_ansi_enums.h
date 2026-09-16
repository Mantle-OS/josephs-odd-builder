#pragma once

namespace job::ansi::utils::enums {

enum class AnsiSequenceType {
    CSI,        // \x1B[
    DECSET,     // \x1B[?
    OSC,        // \x1B]
    ESC,        // \x1B + 1 char
    TEXT,       // Normal printable characters
    UNKNOWN
};
// CSI Ps J — Selective Erase in Display
enum class DECSED_MODE {
    ERASE_BELOW     = 0,  // Erase from cursor to end of screen
    ERASE_ABOVE     = 1,  // Erase from top of screen to cursor
    ERASE_ALL       = 2,  // Erase entire screen
    ERASE_SCROLLBACK = 3  // Erase scrollback buffer (xterm extension)
};

// CSI Ps K — Selective Erase in Line
enum class DECSEL_MODE {
    ERASE_RIGHT = 0, // Erase from cursor to end of line
    ERASE_LEFT  = 1, // Erase from start of line to cursor
    ERASE_LINE  = 2  // Erase entire line
};
// CSI n J — Erase in Display (ED)
enum class ED_MODE {
    TO_END         = 0,  // Erase from cursor to end of screen (default)
    TO_START       = 1,  // Erase from start of screen to cursor
    ENTIRE_SCREEN  = 2,  // Erase entire screen
    SCREEN_AND_SCROLLBACK = 3  // Erase screen and scrollback (xterm extension)
};
// CSI n K — Erase in Line (EL)
enum class EL_MODE {
    TO_END        = 0,  // Erase from cursor to end of line (default)
    TO_START      = 1,  // Erase from start of line to cursor
    ENTIRE_LINE   = 2   // Erase entire line
};
enum class AnsiColorIndex : int {
    Black   = 0,
    Red     = 1,
    Green   = 2,
    Yellow  = 3,
    Blue    = 4,
    Magenta = 5,
    Cyan    = 6,
    White   = 7,
    Unknown = -1
};

enum class WINDOW_MANIP_MODE {
    SET_TITLE        = 0,  // Set window title
    SET_ICON         = 1,  // Set icon name/title
    SET_BOTH         = 2,  // Set both window title and icon name
    GET_TITLE        = 20, // Get window title
    GET_ICON         = 21  // Get icon name/title
};

enum class WINDOW_MANIP_CODE {
    DEICONIFY                   = 1,   // De-iconify (restore from minimized)
    ICONIFY                     = 2,   // Iconify (minimize)
    MOVE_WINDOW                 = 3,   // Move window to [x, y]
    RESIZE_CHAR_CELLS           = 4,   // Resize window to [height, width] chars
    RAISE_WINDOW                = 5,   // Raise window to front
    LOWER_WINDOW                = 6,   // Lower window to back
    REFRESH_WINDOW              = 7,   // Refresh window
    RESIZE_PIXELS               = 8,   // Resize window to [height, width] pixels
    MAXIMIZE_WINDOW             = 9,   // Maximize window (full screen)
    UNMAXIMIZE_WINDOW          = 10,   // Restore from maximized
    REPORT_STATE               = 11,   // Report window state
    REPORT_POSITION            = 13,   // Report window position
    REPORT_SIZE_CHAR_CELLS     = 14,   // Report window size in characters
    REPORT_SIZE_PIXELS         = 18,   // Report window size in pixels
    REPORT_ICON_LABEL          = 19,   // Report icon label (title)
    REPORT_TITLE               = 20,   // Report window title
    REPORT_WINDOW_STATE        = 21,   // Report window state (different format)
    RESTORE_WINDOW             = 22,   // Restore window from minimized/maximized
    PUSH_TITLE                 = 24,   // Push title on stack (was 22)
    POP_TITLE                  = 23    // Pop title from stack
};

enum class TBC_MODE {
    CLEAR_CURRENT = 0, // Clear tab stop at current column
    CLEAR_ALL     = 3  // Clear all tab stops
};


// Standard DSR (Device Status Report) modes
enum DSR_MODE {
    DSR_STATUS             = 5,  // Reports "OK" status → ESC [ 0 n
    DSR_CURSOR             = 6,  // Reports cursor position → ESC [ row ; col R

    // Extended responses for modern terminals (some are xterm-specific)
    DSR_EXT_IDENT         = 0,   // xterm extension: query terminal identity (XTVERSION)
    DSR_PRINTER_STATUS    = 10,  // Reports printer status (legacy DEC)
    DSR_UDK_STATUS        = 11,  // Report user-defined keys (not used by most terms)
    DSR_MEMORY_CHECKSUM   = 12,  // Report memory checksum (VT420+)
    DSR_MICRO_STATUS      = 13,  // Report micro-feature status (rare)
    DSR_HELP_SCREEN       = 14,  // Report help screen status (rare)
    DSR_KEYBOARD_LANG     = 15,  // Report keyboard language (e.g., VT520)
    DSR_EXT_MODE_REPORT   = 16,  // Report mode status (DEC, not widely supported)
    DSR_PRINTER_CONFIG    = 17,  // Printer configuration report
    DSR_LOCALE_REPORT     = 18,  // Report terminal locale
    DSR_TERMINAL_STATE    = 19,  // Request terminal state report

    // Reserved or unsupported
    DSR_RESERVED_20       = 20,  // Reserved by DEC
    DSR_RESERVED_21       = 21,  // Reserved by DEC

    // Used by DEC with private marker (e.g. CSI ? 25 n = cursor visibility)
    // These use DECSET_PRIVATE_CODE instead!
};

// DECSET = DEC Public Mode Set
enum class DECSET_PUBLIC_CODE : int {
    ERROR_HANDLING_MODE         = 0,   // Error handling mode (often ignored)
    KEYBOARD_ACTION_MODE        = 2,   // Lock keyboard input (rare)
    INSERT_MODE                 = 4,   // Enables insert mode (characters pushed forward)

    FORMAT_EFFECTOR_ACTION_1    = 10,  // Handle FE chars (LF, CR, FF) as control
    FORMAT_EFFECTOR_ACTION_2    = 11,  // Alternate FE mode
    SEND_RECEIVE_MODE           = 12,  // Full-duplex (off = local echo)
    LINE_FEED_MODE              = 18,  // CR+LF handling
    AUTO_NEWLINE_WRAP           = 19,  // Auto move to next line at right margin
};


// DECSET = DEC Private Mode Set
enum class DECSET_PRIVATE_CODE : int {
    CURSOR_KEYS             = 1,      // Application Cursor Keys (ESC [ ? 1 h/l)
    COLUMN_MODE_132         = 3,      // Switch between 80/132-column mode
    SMOOTH_SCROLL           = 4,      // Smooth scrolling (mostly ignored)
    REVERSE_VIDEO           = 5,      // Reverse video (invert fg/bg colors)
    ORIGIN_MODE             = 6,      // Cursor origin relative to scroll region
    WRAPAROUND_MODE         = 7,      // Line wraparound when cursor hits edge
    AUTOREPEAT_MODE         = 8,      // Enable keyboard auto-repeat
    INTERLACE_MODE          = 9,      // Interlaced display mode (obsolete)
    BLINKING_CURSOR         = 12,     // Blink the visible cursor
    CURSOR_VISIBLE          = 25,     // Show/hide text cursor
    MOUSE_X10               = 1000,   // Mouse button press only (X10 mode)
    MOUSE_VT200_HIGHLIGHT   = 1001,   // Mouse highlight tracking (rare)
    MOUSE_BTN_EVENT         = 1002,   // Mouse button press/release
    MOUSE_ANY_EVENT         = 1003,   // All mouse events including motion
    FOCUS_EVENT             = 1004,   // Focus in/out reporting
    MOUSE_UTF8              = 1005,   // UTF-8 mouse coordinates
    MOUSE_SGR_EXT           = 1006,   // SGR extended mouse format
    MOUSE_ALTERNATE_SCROLL  = 1007,   // Scroll events send arrow keys in alt screen
    BRACKETED_PASTE         = 2004,   // Wrap pasted text in markers (200~ / 201~)
    ALTERNATE_SCREEN        = 1047,   // Switch to alternate screen buffer
    SAVE_RESTORE_CURSOR     = 1048,   // Save/restore cursor state
    ALTERNATE_SCREEN_EXT    = 1049,   // Alt screen + save/restore combo
    UNKNOWN                 = -1      // Fallback for unknown code
};



// CSI = Control Sequence Introducer
enum class CSI_CODE : char {
    ICH                         = '@',          // Insert Characters
    CUU                         = 'A',          // Cursor Up
    CUD                         = 'B',          // Cursor Down
    CUF                         = 'C',          // Cursor Forward
    CUB                         = 'D',          // Cursor Back
    CNL                         = 'E',          // Cursor Next Line
    CPL                         = 'F',          // Cursor Previous Line
    CHA                         = 'G',          // Cursor Horizontal Absolute
    CHT                         = 'I',          // Cursor Forward Tab
    HPR                         = 'a',          // Horizontal Position Relative
    CUP                         = 'H',          // Cursor Position (row;col)
    ED                          = 'J',          // Erase in Display
    EL                          = 'K',          // Erase in Line
    IL                          = 'L',          // Insert Line
    DL                          = 'M',          // Delete Line
    EF                          = 'N',          // Erase in Field
    EA                          = 'O',          // Erase in Area
    DCH                         = 'P',          // Delete Characters
    SEE                         = 'Q',          // Select Editing Extent
    CPR                         = 'R',          // Cursor Position Report
    SU                          = 'S',          // Scroll Up
    SD                          = 'T',          // Scroll Down
    NP                          = 'U',          // Next Page
    PP                          = 'V',          // Previous Page
    CTC                         = 'W',          // Cursor Tab Control
    ECH                         = 'X',          // Erase Characters
    CBT                         = 'Z',          // Cursor Backward Tab
    HPA                         = 'G',          // Horizontal Position Absolute
    REP                         = 'b',          // Repeat preceding character
    DA                          = 'c',          // Device Attributes
    VPA                         = 'd',          // Vertical Position Absolute
    HVP                         = 'f',          // Horizontal and Vertical Position
    TBC                         = 'g',          // Tab Clear
    SM                          = 'h',          // Set Mode
    MC                          = 'i',          // Media Copy
    RM                          = 'l',          // Reset Mode
    SGR                         = 'm',          // Select Graphic Rendition
    DSR                         = 'n',          // Device Status Report
    SCP                         = 's',          // Save Cursor Position
    RCP                         = 'u',          // Restore Cursor Position
    WINDOW_MANIP                = 't',          // DECSLPP / window manipulation
    DECSCUSR                    = 'q',          // Set cursor style
    DECSCA                      = '\"',         // Select character protection
    DECCRA                      = '#',          // Copy rectangular area
    DECLL                       = 'p',          // Load LED
    DA2                         = '>',          // Secondary device attributes
    DA3                         = '=',          // Tertiary device attributes
    DECSTBM                     = 'r',          // Set scrolling region
    DECSWL                      = 'w',          // Single width line
    DECDWL                      = 'v',          // Double width line
    DECDHL_TOP                  = '3',          // Double height line (top)
    DECDHL_BOTTOM               = '4',          // Double height line (bottom)
    DECFRA                      = 'z',          // Fill rectangular area
    DECRARA                     = '{',          // Reverse assign color
    DECARA                      = 'z',          // Assign color
    SL                          = 'X',          // Scroll left
    SR                          = 'Y',          // Scroll right
    SCP_DECSC                   = 's',          // Save cursor/attrs
    RCP_DECRC                   = 'u',          // Restore cursor/attrs


    TILDE = '~',                                // Used by CSI sequences like CSI 200~ and CSI 201~ (e.g. bracketed paste)

    PASTE_BEGIN                 = '{',          // Bracketed paste start — internal use
    PASTE_END                   = '|',          // Bracketed paste end — internal use

    UNKNOWN = '\0'      // Fallback
};




// SGR
enum class SGR_CODE : int {
    RESET           = 0,   // All attributes off
    BOLD            = 1,   // Bold or increased intensity
    FAINT           = 2,   // Faint, decreased intensity (often not rendered)
    ITALIC          = 3,   // Italicized
    UNDERLINE       = 4,   // Single underline
    SLOW_BLINK      = 5,   // Slow blink (less than 150 per minute)
    RAPID_BLINK     = 6,   // Rapid blink (MS-DOS legacy)
    INVERSE         = 7,   // Swap foreground/background
    CONCEAL         = 8,   // Hide text (used in password prompts)
    STRIKETHROUGH   = 9,   // Strikethrough

    // 10–19: Font selection (rare, usually ignored)
    FONT_DEFAULT    = 10,
    FONT_ALT_1      = 11,
    FONT_ALT_2      = 12,
    FONT_ALT_3      = 13,
    FONT_ALT_4      = 14,
    FONT_ALT_5      = 15,
    FONT_ALT_6      = 16,
    FONT_ALT_7      = 17,
    FONT_ALT_8      = 18,
    FONT_ALT_9      = 19,

    // 20–29: More text decoration
    FRAKTUR         = 20, // Rare (not widely supported)
    DOUBLY_UNDERLINE = 21,
    NORMAL_INTENSITY = 22, // Not bold or faint
    NO_ITALIC       = 23,
    NO_UNDERLINE    = 24,
    NO_BLINK        = 25,
    NO_INVERSE      = 27,
    NO_CONCEAL      = 28,
    NO_STRIKETHROUGH = 29,

    // 30–37: Foreground colors (normal)
    FG_BLACK        = 30,
    FG_RED          = 31,
    FG_GREEN        = 32,
    FG_YELLOW       = 33,
    FG_BLUE         = 34,
    FG_MAGENTA      = 35,
    FG_CYAN         = 36,
    FG_WHITE        = 37,

    EXTENDED_FG     = 38, // extended FG (256/truecolor)

    // 39 = default FG
    FG_DEFAULT      = 39,

    // 40–47: Background colors (normal)
    BG_BLACK        = 40,
    BG_RED          = 41,
    BG_GREEN        = 42,
    BG_YELLOW       = 43,
    BG_BLUE         = 44,
    BG_MAGENTA      = 45,
    BG_CYAN         = 46,
    BG_WHITE        = 47,

    EXTENDED_BG     =  48, //extended_BG (256/truecolor)

    // 49 = default BG
    BG_DEFAULT      = 49,

    // 50 - 55
    FRAME                 = 51,
    ENCIRCLED             = 52,
    OVERLINE              = 53,
    NO_FRAME_NO_CIRCLE    = 54,
    NO_OVERLINE           = 55,

    // 90–97: Bright foreground
    FG_BRIGHT_BLACK   = 90,
    FG_BRIGHT_RED     = 91,
    FG_BRIGHT_GREEN   = 92,
    FG_BRIGHT_YELLOW  = 93,
    FG_BRIGHT_BLUE    = 94,
    FG_BRIGHT_MAGENTA = 95,
    FG_BRIGHT_CYAN    = 96,
    FG_BRIGHT_WHITE   = 97,

    // 100–107: Bright background
    BG_BRIGHT_BLACK   = 100,
    BG_BRIGHT_RED     = 101,
    BG_BRIGHT_GREEN   = 102,
    BG_BRIGHT_YELLOW  = 103,
    BG_BRIGHT_BLUE    = 104,
    BG_BRIGHT_MAGENTA = 105,
    BG_BRIGHT_CYAN    = 106,
    BG_BRIGHT_WHITE   = 107
};



// OSC
enum class OSC_CODE : int {
    WINDOW_ICON             = 0,   // Set icon name (obsolete)
    WINDOW_TITLE            = 1,   // Set window title (deprecated in favor of 2)
    WINDOW_TITLE2           = 2,   // Set window title (main xterm usage)
    SET_HYPERLINK           = 8,   // Set hyperlink anchor
    SELECT_FOREGROUND_COLOR = 10,  // Set default foreground
    SELECT_BACKGROUND_COLOR = 11,  // Set default background
    SELECT_CURSOR_COLOR     = 12,  // Set cursor color
    SET_CLIPBOARD           = 52,  // Set clipboard contents (base64)
    RESET_FG_BG             = 104, // Reset FG and BG to defaults
    RESET_CURSOR_COLOR      = 112, // Reset cursor color
    RESET_SELECTION_COLOR   = 113, // Reset highlight color
    RESET_CLIPBOARD         = SET_CLIPBOARD,  // Use with `;clear` to clear clipboard
    UNKNOWN                 = -1   // Catch-all for unhandled OSC
};


// ESC <char> — Single-character escape sequences
enum class ESC_CODE : char {
    SELECT_CHARSET_G0 = '(', // ESC ( <designator> → G0 character set
    SELECT_CHARSET_G1 = ')', // ESC ) <designator> → G1 character set
    SELECT_CHARSET_G2 = '*', // ESC * <designator> → G2
    SELECT_CHARSET_G3 = '+', // ESC + <designator> → G3

    ENABLE_APP_KEYPAD   = '=', // Application keypad ON
    DISABLE_APP_KEYPAD  = '>', // Application keypad OFF

    RESET_TO_INITIAL    = 'c', // RIS - full terminal reset
    SAVE_CURSOR         = '7', // DECSC - save cursor position
    RESTORE_CURSOR      = '8', // DECRC - restore cursor position

    // ESC D (IND): Index (move cursor down)
    INDEX               = 'D',

    // ESC E (NEL): Next Line
    NEXT_LINE           = 'E',

    // ESC H (HTS): Set tab stop
    SET_TAB_STOP        = 'H',

    // ESC M (RI): Reverse Index (move cursor up)
    REVERSE_INDEX       = 'M',

    // ESC Z: Identify terminal (DECID)
    IDENTIFY_TERMINAL   = 'Z',

    UNKNOWN             = '\0'
};

// Final byte of a sequence like ESC ( B
enum class CharsetDesignator : char {
    US_ASCII             = 'B',  // ASCII
    UK                   = 'A',  // ISO646-GB
    DEC_SPECIAL_GRAPHICS = '0',  // DEC line drawing
    DEC_SUPPLEMENTAL     = '<',  // DEC Supplemental Graphics
    DEC_TECHNICAL        = '>',  // DEC Supplemental Graphics
    ISO_LATIN_1          = 'C',  // ISO8859-1
    ISO_LATIN_2          = 'D',  // ISO8859-2
    JIS_ROMAN            = 'J',  // JIS X 0201 Roman
    JIS_KATAKANA         = 'I',  // JIS X 0201 Katakana
    // Add other designators as needed
    UNUSED               = '\0'  // Fallback
};



enum class CSI_CATEGORY : int {
    CSI_CURSOR          = 0,    // Cursor movement, save/restore, origin mode (CUP, CUU, SCP, RCP)
    CSI_SCROLL          = 1,    // Scrolling and scroll region (IL, DL, DECSTBM)
    CSI_CHAR_ATTR       = 2,    // Text attributes and character protection (SGR, DECSCA, DECERA)
    CSI_TABULATION      = 3,    // Horizontal movement and tab stops (HTS, CHT, CBT, TBC)
    CSI_DEVICE_STATUS   = 4,    // Status device reporting (DA, DA2, DA3, DSR, CPR, MC)
    CSI_MODE_CHANGE     = 5,    // Set reset modes (SM, RM, DECSM, DECRST, DECSET, DECRQM)
    CSI_RECT_OPS        = 6,    // Rectangular operations (DECFRA, DECCRA, DECARA, DECRARA)
    CSI_WINDOWOPS       = 7,    // Window manipulation and queries (CSI t, resize/move/report)
    CSI_CHAR_WIDTH      = 8,    // Line height/width (DECSWL, DECDWL, DECDHL)
    CSI_XTERM_EXT       = 9,    // XTerm-specific extensions (like OSC 1337 or CSI > Ps;...c)
    CSI_UNCLASSIFIED    = 10    // Miscellaneous or vendor-specific (DECLL, LED, aliases, unknowns)
};

// XTerm Mouse Tracking Modes
enum class XTERM_MOUSE_MODE : int {
    NORMAL_TRACKING     = 1000,  // Basic mouse click reporting
    HIGHLIGHT_TRACKING  = 1001,  // Highlight tracking
    BUTTON_EVENT        = 1002,  // Button events (press and release)
    ANY_EVENT           = 1003,  // Any mouse event (including motion)
    FOCUS_EVENT         = 1004,  // Report focus in/out events
    UTF8_COORDS         = 1005,  // UTF-8 coordinate encoding
    SGR_COORDS          = 1006,  // SGR coordinate encoding
    URXVT_COORDS        = 1015,  // URXVT coordinate encoding
    ALTERNATE_SCROLL    = 1007,  // Alternate scroll mode
    BRACKETED_PASTE     = 2004   // Bracketed paste mode
};

// XTerm Mouse Encoding Format
enum class XTERM_MOUSE_ENCODING : int {
    DEFAULT = 0,    // Default X10 encoding
    UTF8    = 1005, // UTF-8 encoding
    SGR     = 1006, // SGR encoding
    URXVT   = 1015  // URXVT encoding
};

// XTerm Feature Reporting
enum class XTERM_FEATURE : int {
    MOUSE_SUPPORT     = 1000, // Mouse protocol support
    FOCUS_SUPPORT     = 1004, // Focus event support
    PASTE_SUPPORT     = 2004, // Bracketed paste support
    COLOR_SUPPORT     = 4000, // Extended color support
    TITLE_SUPPORT     = 14,   // Title stack operations
    FONT_SUPPORT      = 20,   // Font operations
    TEK_SUPPORT       = 30,   // Tektronix 4014 support
    RESIZE_SUPPORT    = 40,   // Window resize operations
    LOGGING_SUPPORT   = 46,   // Logging operations
    CHARSET_SUPPORT   = 50,   // Character set support
    SIXEL_SUPPORT     = 80,   // Sixel graphics support
    DECUDK_SUPPORT    = 90,   // User-defined keys
    DECPRO_SUPPORT    = 95    // DEC private modes
};

enum class UnderlineStyle : int {
    None    = 0,
    Single  = 1,
    Double  = 2,
    Curly   = 3,
    Dotted  = 4,
    Dashed  = 5
};

enum class CursorStyle {
    BlinkingBlock       = 1,  // CSI 0 q
    SteadyBlock         = 2,  // CSI 2 q
    BlinkingUnderline   = 3,  // CSI 3 q
    SteadyUnderline     = 4,  // CSI 4 q
    BlinkingBar         = 5,  // CSI 5 q
    SteadyBar           = 6   // CSI 6 q
};


enum class SEE_MODE {
    LINE = 0,
    DISPLAY = 1
};

enum class DECSCA_MODE {
    NONE = 0,
    ERASE_PROTECTED = 1,
    WRITE_ERASE_PROTECTED = 2
};

enum class EF_MODE {
    TO_END = 0,
    TO_START = 1,
    ENTIRE_LINE = 2
};
enum class EA_MODE {
    TO_END = 0,
    TO_START = 1,
    ENTIRE_SCREEN = 2
};

enum class TILDE_MODE {
    BRACKETED_PASTE_START_CODE = 200,
    BRACKETED_PASTE_END_CODE = 201
};

};
