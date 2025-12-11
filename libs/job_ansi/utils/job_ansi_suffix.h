#pragma once
#include <string>

namespace job::ansi::utils::suffix {

// Canonical escape sequences
inline const std::string ESC  = "\x1B";        // Escape
inline const std::string CSI  = ESC + "[";     // Control Sequence Introducer
inline const std::string OSC  = ESC + "]";     // Operating System Command
inline const std::string DCS  = ESC + "P";     // Device Control String
inline const std::string ST   = ESC + "\\";    // String Terminator
inline const std::string BEL  = "\a";          // Bell

// --- SGR (Select Graphic Rendition) ---
inline const std::string SGR_SUFFIX = "m";
inline const std::string RESET_SGR  = CSI + "0" + SGR_SUFFIX;

// --- Arrow Key Sequences (VT100) ---
inline const std::string ARROW_UP    = CSI + "A";
inline const std::string ARROW_DOWN  = CSI + "B";
inline const std::string ARROW_RIGHT = CSI + "C";
inline const std::string ARROW_LEFT  = CSI + "D";

// --- Page and Navigation Keys ---
inline const std::string PAGE_UP     = CSI + "5~";
inline const std::string PAGE_DOWN   = CSI + "6~";

// --- Control Characters ---
inline const char CTRL_C    = 3;     // End of Text (ETX), Ctrl+C
inline const char ESC_KEY   = 27;    // Escape ASCII code
inline const char TAB       = '\t';  // Horizontal tab
inline const char SPACEBAR  = ' ';   // Space character

inline const std::string CR       = "\r";     // Carriage Return
inline const std::string LF       = "\n";     // Line Feed
inline const std::string CRLF     = "\r\n";

// --- Cursor Movement ---
inline const std::string CURSOR_HOME     = CSI + "H";
inline const std::string CURSOR_HIDE     = CSI + "?25l";
inline const std::string CURSOR_SHOW     = CSI + "?25h";
inline const std::string CURSOR_SAVE     = CSI + "s";
inline const std::string CURSOR_RESTORE  = CSI + "u";

// --- Screen Clearing ---
inline const std::string CLEAR_SCREEN     = CSI + "2J";
inline const std::string CLEAR_LINE       = CSI + "2K";
inline const std::string CLEAR_LINE_RIGHT = CSI + "0K";
inline const std::string CLEAR_LINE_LEFT  = CSI + "1K";
inline const std::string CLEAR_AND_HOME   = CLEAR_SCREEN + CURSOR_HOME;
inline const std::string CLEAR_SCROLLBACK   = CSI + "3J";     // Erase saved lines (scrollback)

// --- Alternate Screen Buffer ---
inline const std::string ENABLE_ALT_SCREEN  = CSI + "?1049h"; // Switch to alternate buffer (saves main)
inline const std::string DISABLE_ALT_SCREEN = CSI + "?1049l"; // Restore main buffer

// --- DEC Private Mode Sequences ---
inline const std::string DECSET_ESC = ESC + "[?";

// --- Mouse Tracking (examples) ---
inline const std::string ENABLE_MOUSE_X10   = CSI + "?1000h";
inline const std::string DISABLE_MOUSE_X10  = CSI + "?1000l";

inline const std::string ENABLE_MOUSE_BUTTON_EVENT = CSI + "?1002h";
inline const std::string DISABLE_MOUSE_BUTTON_EVENT = CSI + "?1002l";

inline const std::string ENABLE_MOUSE_SGR = CSI + "?1006h";
inline const std::string DISABLE_MOUSE_SGR = CSI + "?1006l";

// --- Combined Mouse Tracking ---
inline const std::string ENABLE_MOUSE_ALL = ENABLE_MOUSE_X10 + ENABLE_MOUSE_BUTTON_EVENT + ENABLE_MOUSE_SGR;

inline const std::string DISABLE_MOUSE_ALL = DISABLE_MOUSE_X10 + DISABLE_MOUSE_BUTTON_EVENT + DISABLE_MOUSE_SGR;

}
