#include "osc.h"

#include <iostream>

#include "utils/rgb_color.h"

namespace job::ansi::csi {

DispatchOSC::DispatchOSC(Screen *screen) :
    m_screen{screen}
{
}

void DispatchOSC::dispatch(OSC_CODE code, const std::string &data)
{
    switch (code) {
    case OSC_CODE::WINDOW_ICON:
        // std::cerr << "Set icon title: " << data << '\n';
        m_screen->set_windowIconTitle(data);
        m_screen->set_windowTitle(data);
        break;
    case OSC_CODE::WINDOW_TITLE:
    case OSC_CODE::WINDOW_TITLE2:
        // std::cerr << "Set window title: " << data << '\n';
        m_screen->set_windowTitle(data);
        break;
    case OSC_CODE::SET_HYPERLINK:
        // std::cerr << "Set hyperlink anchor: " << data << '\n';
        m_screen->set_hyperlinkAnchor(data);
        break;
    case OSC_CODE::SELECT_FOREGROUND_COLOR: {
        auto maybeColor = utils::parseColorString(data);
        if (maybeColor)
            m_screen->currentCell()->setForeground(*maybeColor);
        break;
    }
    case OSC_CODE::SELECT_BACKGROUND_COLOR: {
        auto maybeColor = utils::parseColorString(data);
        if (maybeColor)
            m_screen->currentCell()->setBackground(*maybeColor);
        break;
    }
    case OSC_CODE::SELECT_CURSOR_COLOR: {
        if (auto maybeColor = utils::parseColorString(data))
            m_screen->cursor()->setCursorColor(data);
        else
            m_screen->cursor()->resetCursorColor(); // invalid input, revert to default
        break;
    }
    case OSC_CODE::SET_CLIPBOARD:
        // std::cerr << "Set clipboard content: " << data << '\n';
        m_screen->set_clipboardData(data);
        break;
    case OSC_CODE::RESET_FG_BG:{
        // std::cerr << "Reset foreground/background colors to defaults\n";
        auto cell = m_screen->currentCell();
        cell->setForeground(utils::RGBColor{0, 255, 0});
        cell->setBackground(utils::RGBColor{0, 0, 0} );
        break;
    }
    case OSC_CODE::RESET_CURSOR_COLOR: {
        m_screen->cursor()->resetCursorColor(); // Explicit reset
        break;
    }
    case OSC_CODE::RESET_SELECTION_COLOR:
        // std::cerr << "Reset selection color — not implemented\n";
        break;
    default:
        std::cerr << "Unhandled OSC code: " << static_cast<int>(code)
                  << " with data: " << data << '\n';
        break;
    }
}

}
