#pragma once
#include <string_view>
#include <cstdint>
#include <cstddef>

namespace job::ansi::grammar{

// SequenceType
// Canonical escape sequences
inline constexpr char esc  = "\x1B";        // Escape
inline constexpr char csi  = esc + "[";     // Control Sequence Introducer
inline constexpr char osc  = esc + "]";     // Operating System Command
inline constexpr char dcs  = esc + "P";     // Device Control String
inline constexpr char st   = esc + "\\";    // String Terminator
inline constexpr char bel  = "\a";          // Bell

// --- SGR (Select Graphic Rendition) ---
inline constexpr char sgr_suffix = "m";
inline constexpr char reset_sgr  = csi + "0" + sgr_suffix;

// --- Arrow Key Sequences (VT100) ---
inline constexpr char arrow_up    = csi + "A";
inline constexpr char arrow_down  = csi + "B";
inline constexpr char arrow_right = csi + "C";
inline constexpr char arrow_left  = csi + "D";

// --- Page and Navigation Keys ---
inline constexpr char page_up     = csi + "5~";
inline constexpr char page_down   = csi + "6~";

// --- Control Characters ---
inline constexpr char ctrl_c    = 3;        // End of Text (ETX), Ctrl+C
inline constexpr char esc_key   = 27;       // Escape ASCII code //  DUMB even manager will holw all this shit
inline constexpr char tab       = '\t';     // Horizontal tab
inline constexpr char spacebar  = ' ';      // Space character
inline constexpr char cr        = "\r";     // Carriage Return
inline constexpr char lf        = "\n";     // Line Feed
inline constexpr char crlf      = "\r\n";

// --- Cursor Movement ---
inline constexpr char cursor_home     = csi + "H";
inline constexpr char cursor_hide     = csi + "?25l";
inline constexpr char cursor_show     = csi + "?25h";
inline constexpr char cursor_save     = csi + "s";
inline constexpr char cursor_restore  = csi + "u";

// --- Screen Clearing ---
inline constexpr char clear_screen     = csi + "2J";
inline constexpr char clear_line       = csi + "2K";
inline constexpr char clear_line_right = csi + "0K";
inline constexpr char clear_line_left  = csi + "1K";
inline constexpr char clear_and_home   = clear_screen + cursor_home;
inline constexpr char clear_scrollback   = csi + "3J";     // Erase saved lines (scrollback)

// --- Alternate Screen Buffer ---
inline constexpr char enable_alt_screen  = csi + "?1049h"; // Switch to alternate buffer (saves main)
inline constexpr char disable_alt_screen = csi + "?1049l"; // Restore main buffer

// --- DEC Private Mode Sequences ---
inline constexpr char decset_esc = esc + "[?";

// --- Mouse Tracking  ---
// "\033[M"; MOUSE
inline constexpr char enable_mouse_x10              = csi + "?1000h";
inline constexpr char disable_mouse_x10             = csi + "?1000l";
inline constexpr char enable_mouse_button_event     = csi + "?1002h";
inline constexpr char disable_mouse_button_event    = csi + "?1002l";
inline constexpr char enable_mouse_sgr              = csi + "?1006h";
inline constexpr char disable_mouse_sgr             = csi + "?1006l";
inline constexpr char enable_mouse_all              = enable_mouse_x10 + enable_mouse_button_event + enable_mouse_sgr;
inline constexpr char disable_mouse_all             = disable_mouse_x10 + disable_mouse_button_event + disable_mouse_sgr;

// "\033[<" SGR
// "\033[I"; FOCUS
// "\033[O"  NOT_FOCUS


// enum class CharsetDesignator : char {
inline constexpr char        us_ascii             = 'B';  // ASCII
inline constexpr char        uk                   = 'A';  // ISO646-GB
inline constexpr char        dec_special_graphics = '0';  // DEC line drawing
inline constexpr char        dec_supplemental     = '<';  // DEC Supplemental Graphics
inline constexpr char        dec_technical        = '>';  // DEC Supplemental Graphics
inline constexpr char        iso_latin_1          = 'C';  // ISO8859-1
inline constexpr char        iso_latin_2          = 'D';  // ISO8859-2
inline constexpr char        jis_roman            = 'J';  // JIS X 0201 Roman
inline constexpr char        jis_katakana         = 'I';  // JIS X 0201 Katakana



// ESC
// enum class ESC_CODE : char {
inline constexpr char       esc_select_charset_g0   = '(';  // ESC ( <designator> -> G0 character set
inline constexpr char       esc_select_charset_g1   = ')';  // ESC ) <designator> -> G1 character set
inline constexpr char       esc_select_charset_g2   = '*';  // ESC * <designator> -> G2
inline constexpr char       esc_select_charset_g3   = '+';  // ESC + <designator> -> G3
inline constexpr char       esc_enable_app_keypad   = '=';  // Application keypad ON
inline constexpr char       esc_disable_app_keypad  = '>';  // Application keypad OFF
inline constexpr char       esc_reset_to_initial    = 'c';  // RIS - full terminal reset
inline constexpr char       esc_save_cursor         = '7';  // DECSC - save cursor position
inline constexpr char       esc_restore_cursor      = '8';  // DECRC - restore cursor position
inline constexpr char       esc_index               = 'D';  // ESC D (IND): Index (move cursor down)
inline constexpr char       esc_next_line           = 'E';  // ESC E (NEL): Next Line
inline constexpr char       esc_set_tab_stop        = 'H';  // ESC H (HTS): Set tab stop
inline constexpr char       esc_reverse_index       = 'M';  // ESC M (RI): Reverse Index (move cursor up)
inline constexpr char       esc_identify_terminal   = 'Z';  // ESC Z: Identify terminal (DECID)
inline constexpr char       esc_unknown             = '\0';


//CSI

// enum class CSI_CODE : char {
inline constexpr char    esc_ich                         = '@';          // Insert Characters
inline constexpr char    esc_cuu                         = 'A';          // Cursor Up
inline constexpr char    esc_cud                         = 'B';          // Cursor Down
inline constexpr char    esc_cuf                         = 'C';          // Cursor Forward
inline constexpr char    esc_cub                         = 'D';          // Cursor Back
inline constexpr char    esc_cnl                         = 'E';          // Cursor Next Line
inline constexpr char    esc_cpl                         = 'F';          // Cursor Previous Line
inline constexpr char    esc_cha                         = 'G';          // Cursor Horizontal Absolute
inline constexpr char    esc_cht                         = 'I';          // Cursor Forward Tab
inline constexpr char    esc_hpr                         = 'a';          // Horizontal Position Relative
inline constexpr char    esc_cup                         = 'H';          // Cursor Position (row;col)
inline constexpr char    esc_ed                          = 'J';          // Erase in Display
inline constexpr char    esc_el                          = 'K';          // Erase in Line
inline constexpr char    esc_il                          = 'L';          // Insert Line
inline constexpr char    esc_dl                          = 'M';          // Delete Line
inline constexpr char    esc_ef                          = 'N';          // Erase in Field
inline constexpr char    esc_ea                          = 'O';          // Erase in Area
inline constexpr char    esc_dch                         = 'P';          // Delete Characters
inline constexpr char    esc_see                         = 'Q';          // Select Editing Extent
inline constexpr char    esc_cpr                         = 'R';          // Cursor Position Report
inline constexpr char    esc_su                          = 'S';          // Scroll Up
inline constexpr char    esc_sd                          = 'T';          // Scroll Down
inline constexpr char    esc_np                          = 'U';          // Next Page
inline constexpr char    esc_pp                          = 'V';          // Previous Page
inline constexpr char    esc_ctc                         = 'W';          // Cursor Tab Control
inline constexpr char    esc_ech                         = 'X';          // Erase Characters
inline constexpr char    esc_cbt                         = 'Z';          // Cursor Backward Tab
inline constexpr char    esc_hpa                         = 'G';          // Horizontal Position Absolute
inline constexpr char    esc_rep                         = 'b';          // Repeat preceding character
inline constexpr char    esc_da                          = 'c';          // Device Attributes
inline constexpr char    esc_vpa                         = 'd';          // Vertical Position Absolute
inline constexpr char    esc_hvp                         = 'f';          // Horizontal and Vertical Position
inline constexpr char    esc_tbc                         = 'g';          // Tab Clear
inline constexpr char    esc_sm                          = 'h';          // Set Mode
inline constexpr char    esc_mc                          = 'i';          // Media Copy
inline constexpr char    esc_rm                          = 'l';          // Reset Mode
inline constexpr char    esc_sgr                         = 'm';          // Select Graphic Rendition
inline constexpr char    esc_dsr                         = 'n';          // Device Status Report
inline constexpr char    esc_scp                         = 's';          // Save Cursor Position
inline constexpr char    esc_rcp                         = 'u';          // Restore Cursor Position
inline constexpr char    esc_window_manip                = 't';          // DECSLPP / window manipulation
inline constexpr char    esc_decscusr                    = 'q';          // Set cursor style
inline constexpr char    esc_decsca                      = '\"';         // Select character protection
inline constexpr char    esc_deccra                      = '#';          // Copy rectangular area
inline constexpr char    esc_decll                       = 'p';          // Load LED
inline constexpr char    esc_da2                         = '>';          // Secondary device attributes
inline constexpr char    esc_da3                         = '=';          // Tertiary device attributes
inline constexpr char    esc_decstbm                     = 'r';          // Set scrolling region
inline constexpr char    esc_decswl                      = 'w';          // Single width line
inline constexpr char    esc_decdwl                      = 'v';          // Double width line
inline constexpr char    esc_decdhl_top                  = '3';          // Double height line (top)
inline constexpr char    esc_decdhl_bottom               = '4';          // Double height line (bottom)
inline constexpr char    esc_decfra                      = 'z';          // Fill rectangular area
inline constexpr char    esc_decrara                     = '{';          // Reverse assign color
inline constexpr char    esc_decara                      = 'z';          // Assign color
inline constexpr char    esc_sl                          = 'X';          // Scroll left
inline constexpr char    esc_sr                          = 'Y';          // Scroll right
inline constexpr char    esc_scp_decsc                   = 's';          // Save cursor/attrs
inline constexpr char    esc_rcp_decrc                   = 'u';          // Restore cursor/attrs
inline constexpr char    esc_tilde                       = '~'; // Used by CSI sequences like CSI 200~ and CSI 201~ (e.g. bracketed paste)
inline constexpr char    esc_paste_begin                 = '{';          // Bracketed paste start — internal use
inline constexpr char    esc_paste_end                   = '|';          // Bracketed paste end — internal use
// };




// tokens kinda
inline constexpr char32_t   AT          = U'@';
inline constexpr char32_t   TILDIA      = U'~';
inline constexpr char32_t   SEMI        = U';';

// 0x20  ELSE ....

// enterCSI     = '['
// enterOSC     = ']'
// enterCharset = arrary[ '(', ')', '*', '+' ];
// Plain Text  = 0x7F



// CSI
// '?'


// OSC
// '\x07'
// '\\'



// invaild_utf8 = U'\uFFFD'


// Primary Device Attributes (DA1)
#ifndef DA1_RESPONSE_OVERRIDE
inline constexpr const char *DA1_RESPONSE = "\033[?1;2c";
#else
inline constexpr const char *DA1_RESPONSE = DA1_RESPONSE_OVERRIDE;
#endif

// Secondary Device Attributes (DA2)
#ifndef DA2_RESPONSE_OVERRIDE
inline constexpr const char *DA2_RESPONSE = "\033[>0;95;0c";
#else
inline constexpr const char *DA2_RESPONSE = DA2_RESPONSE_OVERRIDE;
#endif

// Tertiary Device Attributes (DA3)
#ifndef DA3_RESPONSE_OVERRIDE
inline constexpr const char *DA3_RESPONSE = "\033P!|00000000\033\\";
#else
inline constexpr const char *DA3_RESPONSE = DA3_RESPONSE_OVERRIDE;
#endif


//-----------------------------------------------------------
// Extra things internal to JOB


// "\033[?1;0c"      ) // CSI[DA]
// "\x1B[?1;2c"      ) // ESC IDENTIFY_TERMINAL (DECID)


// Atrributes
inline constexpr std::string_view ul_none   = "none";
inline constexpr std::string_view ul_single = "single";
inline constexpr std::string_view ul_double = "double";
inline constexpr std::string_view ul_curly  = "curly";
inline constexpr std::string_view ul_dotted = "dotted";
inline constexpr std::string_view ul_dashed = "dashed";

inline constexpr std::uint32_t   gRatio      = 0x9e3779b9;




// CURSOR NOTHING

//Attributes

}
