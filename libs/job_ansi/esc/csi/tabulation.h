#pragma once
#include "esc/csi/dispatch_base.h"

#include "utils/job_ansi_enums.h"

#include "job_ansi_screen.h"

namespace job::ansi::csi {


class DispatchTabulation : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchTabulation(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        // CHT - Cursor Horizontal Tab
        m_dispatchMap[CSI_CODE::CHT] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            if (Cursor *cursor = m_screen->cursor()) {
                int col = cursor->col();
                for (int i = 0; i < count; ++i) {
                    do {
                        ++col;
                    } while (col < m_screen->columnCount() && !m_screen->isTabStop(col));
                    if (col >= m_screen->columnCount())
                        break;
                }
                m_screen->setCursor(cursor->row(), col);
            }
        };

        // CBT - Cursor Backward Tab
        m_dispatchMap[CSI_CODE::CBT] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            if (Cursor *cursor = m_screen->cursor()) {
                int col = cursor->col();
                for (int i = 0; i < count && col > 0; ++i) {
                    do {
                        --col;
                    } while (col > 0 && !m_screen->isTabStop(col));
                }
                col = std::max(0, col);
                m_screen->setCursor(cursor->row(), col);
            }
        };

        // TBC - Tab Clear
        m_dispatchMap[CSI_CODE::TBC] = [this](const std::vector<int> &params) {
            int mode = params.empty() ? 0 : params[0];
            if (Cursor *cursor = m_screen->cursor()) {
                switch (mode) {
                case 0:  // Clear tab stop at cursor
                    m_screen->clearTabStop(cursor->col());
                    break;
                case 3:  // Clear all tab stops
                    m_screen->clearAllTabStops();
                    break;
                default:
                    std::cerr << "Unhandled TBC mode: " << mode << '\n';
                    break;
                }
            }
        };

        // CTC - Cursor Tab Control (rarely used)
        m_dispatchMap[CSI_CODE::CTC] = [this](const std::vector<int> &params) {
            int mode = params.empty() ? 0 : params[0];
            if (Cursor *cursor = m_screen->cursor()) {
                switch (mode) {
                case 0:  // Set tab stop at cursor
                    m_screen->setTabStop(cursor->col());
                    break;
                case 2:  // Clear tab stop at cursor
                    m_screen->clearTabStop(cursor->col());
                    break;
                case 5:  // Clear all tab stops
                    m_screen->clearAllTabStops();
                    break;
                default:
                    std::cerr << "Unhandled CTC mode: " << mode << '\n';
                    break;
                }
            }
        };
    }

    // FIXME: ESC HTS handler (set horizontal tab at cursor)
    // Add this in DispatchESC if implementing ESC_CODE::HTS
    // void handleHTS() {
    //     if (Cursor *cursor = m_screen->cursor()) {
    //         m_screen->setTabStop(cursor->col());
    //     }
    // }
};

}
