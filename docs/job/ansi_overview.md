# Job ANSI

ANSI parsing + terminal screen model.

This module builds as `jobansi`.

### ANSIParser
Consumes bytes and updates a `job::ansi::Screen`.

- input is treated as a stream (not “one string = one frame”)
- UTF-8 bytes are decoded into codepoints (`job::ansi::utils::Utf8Decoder`)
- plain printable codepoints go to `Screen::putChar(...)`
- escape sequences are buffered and dispatched

Parser states:
- ground text
- ESC
- CSI (`ESC [` + params + final)
- OSC (`ESC ] ... BEL` or `ESC ] ... ESC \`)
- charset designator sequences

Dispatch is split by sequence type:
- CSI -> `job::ansi::csi::DispatchCSI`
- ESC -> `job::ansi::csi::DispatchESC`
- OSC -> `job::ansi::csi::DispatchOSC`

### Screen
A terminal “screen image” that can be painted elsewhere.

Holds:
- a 2D grid of `Cell` for the active buffer
- an alternate buffer (switchable)
- scrollback as a deque of lines (bounded)
- cursor state (`Cursor`)
- current text attributes (`Attributes`)

Tracks:
- dirty rows / dirty columns per row (for incremental rendering)

Implements common terminal behavior:
- cursor movement + save/restore
- scrolling regions + scroll up/down
- tab stops (bitset)
- origin mode / wraparound / insert mode / auto linefeed
- mouse reporting modes + focus event reporting
- “terminal replies” by writing strings back out through `on_terminalOutput`

Many toggles and string fields are exposed via small callback-backed setters
(using `JOB_ANSI_SCREEN_CALLBACK*` macros).

Examples: window title, cursor visibility,
reverse video, bracketed paste, mouse modes, device attribute replies.


### Cells + attributes (`Cell`, `Attributes`, `Cursor`)
- `Cell` One screen location: codepoint + combining marks + width/trailing flags +
  line display mode (double width/height variants) + protection bits + colors.
- `Attributes` is SGR-ish styling: packed flags (bold/italic/etc), underline style,
  optional fg/bg color. Attributes are interned (flyweight cache).
- `Cursor` tracks row/col plus style + optional custom color, with save/restore.


## Escape handling

`libs/job_ansi/esc/`

### DispatchCSI
CSI handling is decomposed into small dispatchers:
- SGR (style + color)
- cursor moves / reports
- scroll region + scrolling
- tabulation
- mode changes (DECSET + public modes)
- device status queries / replies
- window ops / xterm like extensions
- a catch-all “unclassified” bucket for the odd ones

The parser feeds:
- final code (the terminator char)
- parsed integer params

### DispatchESC
Single-byte ESC final sequences plus charset selection hooks.
- save/restore cursor
- keypad mode toggles
- charset bank selection
end up.

### DispatchOSC
title, icon/, clipboard, hyperlinks, etc routed into the Screen
fields and callbacks.

## Utils
`libs/job_ansi/utils/`:

- `Utf8Decoder`: incremental decode + encode + normalization helpers
- `charWidth(...)`: rough terminal width for codepoints (basic CJK handling)
- `RGBColor` + `color_utils`: named colors, xterm 256 palette mapping, hex parsing,
  lighten/darken helpers, SGR basic/bright mapping
- `CharsetTranslator`: G0..G3 banks + DEC charset maps (special graphics, etc)
- `job_ansi_suffix`: common escape strings for emitting sequences
- `job_ansi_enums`: enum space for CSI/OSC/ESC codes and related modes

