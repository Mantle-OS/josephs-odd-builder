#include "mode_change.h"

#include <job_logger.h>

namespace job::ansi::csi {

void DispatchDECSET::dispatchPrivate(DECSET_PRIVATE_CODE code, bool set)
{
    switch (code) {
    case DECSET_PRIVATE_CODE::CURSOR_KEYS:
        // Application cursor keys mode
        JOB_LOG_DEBUG("[DECSET] Application cursor keys mode {}", (set ? "enabled" : "disabled"));
        m_screen->set_cursorKeysMode(set);
        break;

        // SKIPPED
    case DECSET_PRIVATE_CODE::COLUMN_MODE_132:
        // 132 column mode (ignored in modern terminals)
        JOB_LOG_DEBUG("[DECSET] 132 column mode not supported");
        break;

    // SKIPPED
    // "Smooth scroll" (DECSCLM, aka DEC Private Mode 4) causes the terminal to visually scroll line-by-line as new lines are output,
    // instead of jumping a full screen at once. It's an old behavior from VT terminals and is usually disabled in modern emulators due
    // to performance and limited value. Most emulators (including xterm and Linux consoles) ignore this mode.
    case DECSET_PRIVATE_CODE::SMOOTH_SCROLL:
        // Smooth scrolling mode
        JOB_LOG_DEBUG("[DECSET] Smooth scroll mode not supported");
        break;

    case DECSET_PRIVATE_CODE::REVERSE_VIDEO:
        // Reverse video mode
        m_screen->set_reverseVideo(set);
        break;

    case DECSET_PRIVATE_CODE::ORIGIN_MODE:
        m_screen->set_originMode(set);
        break;

    case DECSET_PRIVATE_CODE::WRAPAROUND_MODE:
        m_screen->set_wraparound(set);
        break;

    case DECSET_PRIVATE_CODE::AUTOREPEAT_MODE:
        // Auto-repeat keys mode
        JOB_LOG_DEBUG("[DECSET] Auto-repeat mode not supported");
        break;

    case DECSET_PRIVATE_CODE::BLINKING_CURSOR:
        m_screen->set_blinkingCursor(set);
        break;

    case DECSET_PRIVATE_CODE::CURSOR_VISIBLE:
        m_screen->set_cursorVisible(set);
        break;

    // MOUSE
    case DECSET_PRIVATE_CODE::MOUSE_X10:
        m_screen->set_mouseX10Mode(set);
        break;
    case DECSET_PRIVATE_CODE::MOUSE_VT200_HIGHLIGHT:
        m_screen->set_mouseVT200Highlight(set);
        break;
    case DECSET_PRIVATE_CODE::MOUSE_BTN_EVENT:
        m_screen->set_mouseButtonEventMode(set);
        break;
    case DECSET_PRIVATE_CODE::MOUSE_ANY_EVENT:
        m_screen->set_mouseAnyEventMode(set);
        break;
    case DECSET_PRIVATE_CODE::MOUSE_UTF8:
        m_screen->set_mouseUtf8Encoding(set);
        break;
    case DECSET_PRIVATE_CODE::MOUSE_SGR_EXT:
        m_screen->set_mouseSgrEncoding(set);
        break;

    case DECSET_PRIVATE_CODE::FOCUS_EVENT:
        m_screen->set_focusEventEnabled(set);
        break;

    case DECSET_PRIVATE_CODE::BRACKETED_PASTE:
        m_screen->set_bracketedPasteEnabled(set);
        break;

    case DECSET_PRIVATE_CODE::ALTERNATE_SCREEN:
        if (set) {
            m_screen->enterAlternateGrid();
        } else {
            m_screen->exitAlternateGrid();
        }
        break;

    case DECSET_PRIVATE_CODE::ALTERNATE_SCREEN_EXT:
        if (set) {
            m_screen->saveCursor();
            m_screen->enterAlternateGrid();
        } else {
            m_screen->exitAlternateGrid();
            m_screen->restoreCursor();
        }
        break;

    case DECSET_PRIVATE_CODE::SAVE_RESTORE_CURSOR:
        if (set) {
            m_screen->saveCursor();
        } else {
            m_screen->restoreCursor();
        }
        break;



    default:
        std::string_view setStr = (set ? "set" : "reset");
        JOB_LOG_DEBUG("[DECSET] Unknown private mode: {} = {}" , static_cast<int>(code), setStr);
        break;
    }
}

void DispatchDECSET::dispatchPublic(DECSET_PUBLIC_CODE code, bool set)
{
    switch (code) {
    case DECSET_PUBLIC_CODE::INSERT_MODE:
        m_screen->set_insertMode(set);
        break;

    case DECSET_PUBLIC_CODE::LINE_FEED_MODE:
        m_screen->set_autoLinefeed(set);
        break;


    default:
        JOB_LOG_DEBUG("[DECSET] Unknown public mode: {} = ", static_cast<int>(code), (set ? "set" : "reset"));
        break;
    }
}

} // namespace CSI

