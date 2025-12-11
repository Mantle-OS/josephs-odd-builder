#pragma once
#include <sstream>

#include "job_ansi_enums.h"
#include "rgb_color.h"

#include "job_ansi_screen.h"
#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {


class DispatchWindowOps : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchWindowOps(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override {
        // Window and Title Operations - CSI Ps ; Ps ; Ps t
        m_dispatchMap[CSI_CODE::WINDOW_MANIP] = [this](const std::vector<int> &params) {
            if (params.empty()) return;

            // First check if it's a window manipulation code
            if (params[0] >= 1 && params[0] <= 24) {
                WINDOW_MANIP_CODE code = static_cast<WINDOW_MANIP_CODE>(params[0]);
                switch (code) {
                case WINDOW_MANIP_CODE::DEICONIFY:
                case WINDOW_MANIP_CODE::ICONIFY:
                case WINDOW_MANIP_CODE::RAISE_WINDOW:
                case WINDOW_MANIP_CODE::LOWER_WINDOW:
                case WINDOW_MANIP_CODE::MAXIMIZE_WINDOW:
                case WINDOW_MANIP_CODE::UNMAXIMIZE_WINDOW:
                case WINDOW_MANIP_CODE::RESTORE_WINDOW:
                    // These are window state changes - just acknowledge them
                    m_screen->set_window_reply(std::string("\033[") + std::to_string(static_cast<int>(code)) + "t");
                    break;

                case WINDOW_MANIP_CODE::MOVE_WINDOW:
                    if (params.size() >= 3) {
                        int x = params[1];
                        int y = params[2];
                        std::ostringstream oss;
                        oss << "\033[3;" << x << ";" << y << "t";
                        m_screen->set_window_reply(oss.str());
                    }
                    break;

                case WINDOW_MANIP_CODE::RESIZE_CHAR_CELLS:
                    if (params.size() >= 3) {
                        int height = params[1];
                        int width = params[2];
                        if (height > 0 && width > 0) {
                            m_screen->resize(height, width);
                            std::ostringstream oss;
                            oss << "\033[8;" << height << ";" << width << "t";
                            m_screen->set_window_reply(oss.str());
                        }
                    }
                    break;

                case WINDOW_MANIP_CODE::REFRESH_WINDOW:
                    m_screen->requestRender();
                    break;

                case WINDOW_MANIP_CODE::RESIZE_PIXELS:
                    if (params.size() >= 3) {
                        // Convert pixels to character cells (approximate)
                        int height = params[1] / 10; // Assuming ~10 pixels per char height
                        int width = params[2] / 6;   // Assuming ~6 pixels per char width
                        if (height > 0 && width > 0) {
                            m_screen->resize(height, width);
                            std::ostringstream oss;
                            oss << "\033[8;" << params[1] << ";" << params[2] << "t";
                            m_screen->set_window_reply(oss.str());
                        }
                    }
                    break;

                case WINDOW_MANIP_CODE::REPORT_STATE:
                    // Report window state: 1=normal, 2=minimized, 3=maximized
                    m_screen->set_window_reply("\033[1t"); // Always report normal state
                    break;

                case WINDOW_MANIP_CODE::REPORT_POSITION:
                    {
                        // Report window position (we report 0,0 as we can't get real coords)
                        std::ostringstream oss;
                        oss << "\033[3;0;0t";
                        m_screen->set_window_reply(oss.str());
                    }
                    break;

                case WINDOW_MANIP_CODE::REPORT_SIZE_CHAR_CELLS:
                    {
                        std::ostringstream oss;
                        oss << "\033[4;" << m_screen->rowCount() << ";" 
                            << m_screen->columnCount() << "t";
                        m_screen->set_window_reply(oss.str());
                    }
                    break;

                case WINDOW_MANIP_CODE::REPORT_SIZE_PIXELS:
                    {
                        // Report approximate pixel dimensions
                        int height = m_screen->rowCount() * 10;
                        int width = m_screen->columnCount() * 6;
                        std::ostringstream oss;
                        oss << "\033[4;" << height << ";" << width << "t";
                        m_screen->set_window_reply(oss.str());
                    }
                    break;

                case WINDOW_MANIP_CODE::REPORT_ICON_LABEL:
                    // Report icon label (empty by default)
                    m_screen->set_window_reply("\033]L\033\\");
                    break;

                case WINDOW_MANIP_CODE::REPORT_TITLE:
                    // Report window title
                    m_screen->set_window_reply("\033]l" + m_screen->get_windowTitle() + "\033\\");
                    break;

                case WINDOW_MANIP_CODE::REPORT_WINDOW_STATE:
                    // Extended window state report (similar to REPORT_STATE)
                    m_screen->set_window_reply("\033[1;1;1t"); // Normal state, no zoom
                    break;

                case WINDOW_MANIP_CODE::PUSH_TITLE:
                    m_screen->pushWindowTitle();
                    break;

                case WINDOW_MANIP_CODE::POP_TITLE:
                    m_screen->popWindowTitle();
                    break;

                default:
                    std::cerr << "[CSI:WINDOW] Unknown window operation: " << static_cast<int>(code) << '\n';
                    break;
                }
            }
            // Then check if it's a title manipulation code
            else {
                WINDOW_MANIP_MODE mode = static_cast<WINDOW_MANIP_MODE>(params[0]);
                std::string title = params.size() > 1 ? std::to_string(params[1]) : "";

                switch (mode) {
                case WINDOW_MANIP_MODE::SET_ICON:
                    m_screen->set_windowIconTitle(title);
                    break;

                case WINDOW_MANIP_MODE::SET_TITLE:
                    m_screen->set_windowTitle(title);
                    break;

                case WINDOW_MANIP_MODE::SET_BOTH:
                    m_screen->set_windowIconTitle(title);
                    m_screen->set_windowTitle(title);
                    break;

                case WINDOW_MANIP_MODE::GET_TITLE:
                    m_screen->set_window_reply("\033]l" + m_screen->get_windowTitle() + "\033\\");
                    break;

                case WINDOW_MANIP_MODE::GET_ICON:
                    m_screen->set_window_reply("\033]L" + m_screen->get_windowIconTitle() + "\033\\");
                    break;

                default:
                    std::cerr << "[CSI:WINDOW] Unknown title operation: " << static_cast<int>(mode) << '\n';
                    break;
                }
            }
        };
    }
};

}
