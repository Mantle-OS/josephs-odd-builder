#pragma once

#include <vector>
#include <iostream>

#include "job_ansi_enums.h"

#include "job_ansi_screen.h"

#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {
class DispatchXtermExt : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchXtermExt(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
         // Bracketed Paste Mode End
        m_dispatchMap[CSI_CODE::WINDOW_MANIP] = [](const std::vector<int> &params) {
            int mode = params.empty() ? 0 : params[0];
            std::cerr << "[CSI:WINDOW_MANIP] Window operation mode: " << mode << '\n';
        };

        m_dispatchMap[CSI_CODE::SCP] = [](const std::vector<int> &params) {
            [[maybe_unused]] int mode = params.empty() ? 0 : params[0];
            std::cerr << "[CSI:SCP] Save cursor parameters\n";
        };

        m_dispatchMap[CSI_CODE::RCP] = [](const std::vector<int> &params) {
            [[maybe_unused]] int mode = params.empty() ? 0 : params[0];
            std::cerr << "[CSI:RCP] Restore cursor parameters\n";
        };

        m_dispatchMap[CSI_CODE::PASTE_END] = [this](const std::vector<int> &) {
            m_screen->set_bracketedPasteEnabled(false);
        };

        // Device Attributes 3 (DA3) - Report terminal features
        m_dispatchMap[CSI_CODE::DA3] = [this](const std::vector<int> &) {
            // Report as modern xterm with common features
            m_screen->set_device_attr_tertiary("\033[>1;95;0c"); // Report as xterm version 95
        };

        // Set Mode (SM) - Handle XTerm specific modes
        m_dispatchMap[CSI_CODE::SM] = [this](const std::vector<int> &params) {
            if (params.empty()) return;
            
            auto mode = static_cast<XTERM_MOUSE_MODE>(params[0]);
            switch (mode) {
                case XTERM_MOUSE_MODE::NORMAL_TRACKING:
                case XTERM_MOUSE_MODE::HIGHLIGHT_TRACKING:
                case XTERM_MOUSE_MODE::BUTTON_EVENT:
                case XTERM_MOUSE_MODE::ANY_EVENT:
                    // Mouse tracking modes - acknowledge but don't implement yet
                    break;
                    
                case XTERM_MOUSE_MODE::FOCUS_EVENT:
                    // Focus in/out reporting - acknowledge but don't implement yet
                    break;
                    
                case XTERM_MOUSE_MODE::UTF8_COORDS:
                case XTERM_MOUSE_MODE::SGR_COORDS:
                case XTERM_MOUSE_MODE::URXVT_COORDS:
                    // Mouse encoding modes - acknowledge but don't implement yet
                    break;
                    
                case XTERM_MOUSE_MODE::BRACKETED_PASTE:
                    m_screen->set_bracketedPasteEnabled(true);
                    break;
                    
                default:
                    // Unknown mode - ignore
                    break;
            }
        };

        // Reset Mode (RM) - Handle XTerm specific mode resets
        m_dispatchMap[CSI_CODE::RM] = [this](const std::vector<int> &params) {
            if (params.empty()) return;
            
            auto mode = static_cast<XTERM_MOUSE_MODE>(params[0]);
            switch (mode) {
                case XTERM_MOUSE_MODE::NORMAL_TRACKING:
                case XTERM_MOUSE_MODE::HIGHLIGHT_TRACKING:
                case XTERM_MOUSE_MODE::BUTTON_EVENT:
                case XTERM_MOUSE_MODE::ANY_EVENT:
                    // Disable mouse tracking modes
                    break;
                    
                case XTERM_MOUSE_MODE::FOCUS_EVENT:
                    // Disable focus reporting
                    break;
                    
                case XTERM_MOUSE_MODE::UTF8_COORDS:
                case XTERM_MOUSE_MODE::SGR_COORDS:
                case XTERM_MOUSE_MODE::URXVT_COORDS:
                    // Mouse encoding modes - acknowledge but don't implement yet
                    break;
                    
                case XTERM_MOUSE_MODE::BRACKETED_PASTE:
                    m_screen->set_bracketedPasteEnabled(false);
                    break;
                    
                default:
                    // Unknown mode - ignore
                    break;
            }
        };

        // Device Status Report (DSR) - Handle XTerm specific queries
        m_dispatchMap[CSI_CODE::DSR] = [this](const std::vector<int> &params) {
            if (params.empty()) return;
            
            switch (params[0]) {
                case DSR_MODE::DSR_EXT_IDENT: {
                    // Report terminal version
                    m_screen->set_device_attr("\033[>1;95;0c");
                    break;
                }
                case DSR_MODE::DSR_TERMINAL_STATE: {
                    // Report bracketed paste mode state
                    std::string response = "\033[?";
                    response += std::to_string(static_cast<int>(XTERM_FEATURE::PASTE_SUPPORT));
                    response += ";";
                    response += m_screen->get_bracketedPasteEnabled() ? "1" : "2";
                    response += "$y";
                    m_screen->set_device_attr(response);
                    break;
                }
                default:
                    // Unknown DSR - report as unsupported
                    m_screen->set_device_attr("\033[0n");
                    break;
            }
        };
    }
};

} // namespace CSI

