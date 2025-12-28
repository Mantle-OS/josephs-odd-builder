# Job TUI

Terminal UI.

This module `jobtui`  links against `job_ansi`, `job_core`, `job_io`, and `job_threads`.

job:
- `job::ansi` provides ANSI strings + attributes + colors etc
- `job::core` provides `job::core::IODevice` for output
- `job::threads` provides `job::threads::JobIoAsyncThread`
- `job::tui` item tree + layout + event loop + drawing helpers
- `job::tui::gui` widgets/containers/layouts

---

## JobTuiCoreApplication

Owns the terminal session:
- enables raw mode
- switches to alt screen
- hides/shows cursor
- runs the main loop (input -> events -> paint)

Rendering is “paint the tree”:
- root item paints first
- top-level children paint after
- output is ANSI cursor moves + styled text written to an job::core::IODevice

There is a dirty flag + a minimum render interval.

## JobTuiObject

Base object with:
- parent pointer
- named children map
- lightweight timer hook (heartbeat + callback)
- event hook (`onEvent`) + overridable handler

Used as the ownership for everything in the tui tree.

## JobTuiItem

Drawable node in the tree.
- geometry (x/y/width/height)
- visibility + focus flags
- ANSI attributes (style)
- anchors
- optional layout engine
- optional mouse handling

Paint:
- items paint themselves
- items paint children
- layout/anchors resolve geometry before paint

## Drawing (`DrawContext`)
Writer for terminal output.
- cursor movement
- styled text output
- rectangles/boxes with a few border styles
- foreground/background color application

Wwrites ANSI escape sequences + text directly to `job::core::IODevice`.

## Event
Input is translated into events:
- key press (including arrow keys)
- escape
- quit (Ctrl+C)
- mouse press/release/move (SGR mouse format)
- focus in/out

Focus routing is handled in the app loop:
- focus changes via tab / shift-tab style behavior
- key events go to the focused item first
- mouse events go to items that accept them

## Layout + anchoring

### JobTuiAnchors
Relative geometry rules:
- edge anchors (top/bottom/left/right)
- center anchors (horizontal/vertical)
- fill / center-in
- margins

resolve an item’s rect based on another item or parent.

### LayoutEngine
Layout plug-in attached to an item.

Current layouts (in `job::tui::gui`):
- linear layout (orientation + spacing)
- row layout
- column layout
- grid layout


## Containers (in `job::tui::gui::containers`)

- `Window`: bordered rect + title, contains children
- `Panel`: basic container surface
- `GroupBox`: labeled container surface

JobTuiItem specializations that paint a frame and host children.

## Elements (in `job::tui::gui::elements`)

Widgets:
- `Label`
- `Button`
- `Checkbox`
- `RadioButton`
- `Rectangle` (supports gradient + border styling)
- `JobTuiMouseArea` (invisible input catcher)

widgets are “paint self + handle events” on top of the core item tree.

## Examples (apps)

`apps/job_ansi_examples/*.cpp` exercises this stack:
- constructs an `IODevice` (usually stdout)
- constructs `JobTuiCoreApplication`
- builds a tree of gui items (windows, buttons, layouts, anchors, mouse tests)
- runs the loop
