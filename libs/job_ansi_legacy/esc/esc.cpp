#include "esc.h"

#include "job_ansi_screen.h"
#include <iostream>

namespace job::ansi::csi {

DispatchESC::DispatchESC(Screen *screen) :
    DispatchBase<ESC_CODE, CharsetDesignator>(screen)
{
    initMap();
}

void DispatchESC::dispatch(ESC_CODE code, CharsetDesignator designator)
{
    if (auto it = m_dispatchMap.find(code); it != m_dispatchMap.end())
        it->second(designator);
    else
        std::cerr << "Unhandled ESC code: " << static_cast<char>(code) << '\n';
}

void DispatchESC::initMap()
{
    m_dispatchMap = {
        {ESC_CODE::SELECT_CHARSET_G0, [this](CharsetDesignator d) {
             if (m_screen->g0Charset() != d) {
                 // std::cerr << "Select G0 charset: " << static_cast<char>(d) << " AFTER " << '\n';
                 m_screen->selectCharset(0, d);
             }
         }},

        {ESC_CODE::SELECT_CHARSET_G1, [this](CharsetDesignator d) {
             // std::cerr << "Select G1 charset: " << static_cast<char>(d) << '\n';
             m_screen->selectCharset(1, d);
         }},

        {ESC_CODE::SELECT_CHARSET_G2, [this](CharsetDesignator d) {
             // std::cerr << "Select G2 charset: " << static_cast<char>(d) << '\n';
             m_screen->selectCharset(2, d);
         }},

        {ESC_CODE::SELECT_CHARSET_G3, [this](CharsetDesignator d) {
             // std::cerr << "Select G3 charset: " << static_cast<char>(d) << '\n';
             m_screen->selectCharset(3, d);
         }},

        {ESC_CODE::ENABLE_APP_KEYPAD, [this](CharsetDesignator) {
             // std::cerr << "Enable application keypad mode\n";
             m_screen->set_appKeypadMode(true);
         }},

        {ESC_CODE::DISABLE_APP_KEYPAD, [this](CharsetDesignator) {
             // std::cerr << "Disable application keypad mode\n";
             m_screen->set_appKeypadMode(false);
         }},

        {ESC_CODE::RESET_TO_INITIAL, [this](CharsetDesignator) {
             // std::cerr << "Full terminal reset (RIS)\n";
             m_screen->reset();
         }},

        {ESC_CODE::SAVE_CURSOR, [this](CharsetDesignator) {
             // std::cerr << "Save cursor position (DECSC)\n";
             m_screen->saveCursor();
         }},

        {ESC_CODE::RESTORE_CURSOR, [this](CharsetDesignator) {
             // std::cerr << "Restore cursor position (DECRC)\n";
             m_screen->restoreCursor();
         }},

        {ESC_CODE::INDEX, [this](CharsetDesignator) {
             // std::cerr << "Index (IND): move cursor down\n";
             m_screen->moveCursor(1, 0);
         }},
        {ESC_CODE::NEXT_LINE, [this](CharsetDesignator) {
             // std::cerr << "Next Line (NEL): CR + LF\n";
             int row = m_screen->cursor()->row() + 1;
             m_screen->setCursor(row, 0);
         }},

        {ESC_CODE::REVERSE_INDEX, [this](CharsetDesignator) {
             // std::cerr << "Reverse Index (RI): move cursor up\n";
             m_screen->moveCursor(-1, 0);
         }},

        {ESC_CODE::SET_TAB_STOP, [this](CharsetDesignator) {
             int col = m_screen->cursor()->column();
             m_screen->setTabStop(col);
             // std::cerr << "Set tab stop at column: " << col << '\n';
         }},

        {ESC_CODE::IDENTIFY_TERMINAL, [this](CharsetDesignator) {
             // std::cerr << "DECID: Identify terminal (ESC Z)\n";
             if (m_screen) {
                 const std::string &reply = m_screen->get_terminal_ident_reply();
                 if (!reply.empty())
                     m_screen->set_terminal_ident_reply(reply);  // We'll define this next
             }
         }},
    };
}
}
