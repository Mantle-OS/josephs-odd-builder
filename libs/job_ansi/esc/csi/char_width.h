#pragma once

#include "job_ansi_enums.h"
#include "job_ansi_screen.h"
#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {
class DispatchCharWidth : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchCharWidth(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        m_dispatchMap[CSI_CODE::DECSWL] = [this]([[maybe_unused]] std::span<const int> params) {
            if (Cursor *cursor = m_screen->cursor())
                m_screen->setLineWidth(cursor->row(), 1);
        };

        m_dispatchMap[CSI_CODE::DECDWL] = [this]([[maybe_unused]] std::span<const int> params) {
            if (Cursor *cursor = m_screen->cursor())
                m_screen->setLineWidth(cursor->row(), 2);
        };

        m_dispatchMap[CSI_CODE::DECDHL_TOP] = [this]([[maybe_unused]] std::span<const int> params) {
            if (Cursor *cursor = m_screen->cursor()) {
                m_screen->setLineHeight(cursor->row(), 2);
                m_screen->setLineHeightPosition(cursor->row(), true);
            }
        };

        m_dispatchMap[CSI_CODE::DECDHL_BOTTOM] = [this]([[maybe_unused]] std::span<const int> params) {
            if (Cursor *cursor = m_screen->cursor()) {
                m_screen->setLineHeight(cursor->row(), 2);
                m_screen->setLineHeightPosition(cursor->row(), false);
            }
        };
    }
};

}
