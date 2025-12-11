#pragma once

#include <vector>
#include <iostream>

#include "utils/job_ansi_enums.h"

#include "job_ansi_screen.h"

#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {
class DispatchCursor : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchCursor(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        // Cursor Up (CUU)
        m_dispatchMap[CSI_CODE::CUU] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->moveCursorRelative(-count, 0, m_screen->originRow(), m_screen->originBottom());
        };

        // Cursor Down (CUD)
        m_dispatchMap[CSI_CODE::CUD] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->moveCursorRelative(count, 0, m_screen->originRow(), m_screen->originBottom());
        };

        // Cursor Forward (CUF)
        m_dispatchMap[CSI_CODE::CUF] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->moveCursorRelative(0, count, m_screen->originRow(), m_screen->originBottom());
        };

        // Cursor Backward (CUB)
        m_dispatchMap[CSI_CODE::CUB] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->moveCursorRelative(0, -count, m_screen->originRow(), m_screen->originBottom());
        };

        // Cursor Next Line (CNL)
        m_dispatchMap[CSI_CODE::CNL] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            int newRow = m_screen->cursor()->row() + count;
            m_screen->setCursor(newRow, 0);
        };

        // Cursor Previous Line (CPL)
        m_dispatchMap[CSI_CODE::CPL] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            int newRow = m_screen->cursor()->row() - count;
            m_screen->setCursor(newRow, 0);
        };


        // Cursor Horizontal Absolute (CHA)
        m_dispatchMap[CSI_CODE::CHA] = [this](const std::vector<int> &params) {
            int col = params.empty() ? 1 : params[0];
            int row = m_screen->cursor()->row();
            m_screen->setCursor(row - m_screen->originRow(), col - 1);
        };

        // Horizontal Position Relative (HPR)
        m_dispatchMap[CSI_CODE::HPR] = [this](const std::vector<int> &params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->moveCursorRelative(0, count, m_screen->originRow(), m_screen->originBottom());
        };

        // Cursor Position (CUP)
        m_dispatchMap[CSI_CODE::CUP] = [this](const std::vector<int> &params) {
            int row = params.size() > 0 ? params[0] - 1 : 0;
            int col = params.size() > 1 ? params[1] - 1 : 0;

            if (m_screen->originMode()) {
                row += m_screen->scrollTop();
            }

            m_screen->setCursor(row, col);
        };

        // Vertical Position Absolute (VPA)
        m_dispatchMap[CSI_CODE::VPA] = [this](const std::vector<int> &params) {
            int row = params.empty() ? 1 : params[0];
            if (m_screen->originMode()) {
                row += m_screen->scrollTop();
            }
            m_screen->setCursor(row - 1, m_screen->cursor()->col());
        };

        // Horizontal and Vertical Position (HVP)
        m_dispatchMap[CSI_CODE::HVP] = [this](const std::vector<int> &params) {
            int row = params.size() > 0 ? params[0] - 1 : 0;
            int col = params.size() > 1 ? params[1] - 1 : 0;

            if (m_screen->originMode()) {
                row += m_screen->scrollTop();
            }

            m_screen->setCursor(row, col);
        };

        // Save Cursor Position (SCP)
        m_dispatchMap[CSI_CODE::SCP] = [this](const std::vector<int> &) {
            std::cerr << "[CSI] SCP: Save Cursor Parameters\n";
            m_screen->saveCursor();
        };

        // Restore Cursor Position (RCP)
        m_dispatchMap[CSI_CODE::RCP] = [this](const std::vector<int> &) {
            std::cerr << "[CSI] RCP: Restore Cursor Parameters\n";
            m_screen->restoreCursor();
        };
    }
};

}
