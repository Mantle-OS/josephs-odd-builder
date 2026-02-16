#pragma once

#include "job_ansi_screen.h"
#include "utils/job_ansi_enums.h"
#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {


class DispatchScroll final : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchScroll(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        m_dispatchMap[CSI_CODE::IL] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            if (Cursor *cursor = m_screen->cursor()) {
                int row = cursor->row();
                m_screen->insertLines(row, count);
            }
        };

        m_dispatchMap[CSI_CODE::DL] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            if (Cursor *cursor = m_screen->cursor()) {
                int row = cursor->row();
                m_screen->deleteLines(row, count);
            }
        };

        m_dispatchMap[CSI_CODE::SU] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->scrollUp(m_screen->scrollTop(), m_screen->scrollBottom(), count);
        };

        m_dispatchMap[CSI_CODE::SD] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->scrollDown(m_screen->scrollTop(), m_screen->scrollBottom(), count);
        };

        m_dispatchMap[CSI_CODE::SR] = [this]([[maybe_unused]]std::span<const int> params) {
            if (Cursor *cursor = m_screen->cursor()) {
                int row = cursor->row();
                if (row == m_screen->scrollTop()) {
                    m_screen->scrollDown(m_screen->scrollTop(), m_screen->scrollBottom(), 1);
                } else {
                    cursor->move(1, 0);
                }
            }
        };

        m_dispatchMap[CSI_CODE::DECSTBM] = [this](std::span<const int> params) {
            int top = params.empty() ? 0 : params[0] - 1;
            int bottom = params.size() < 2 ? m_screen->rowCount() - 1 : params[1] - 1;

            if (m_screen->originRow() > 0) {
                top = std::clamp(top, 0, m_screen->rowCount() - 1);
                bottom = std::clamp(bottom, 0, m_screen->rowCount() - 1);
            }

            m_screen->setScrollRegion(top, bottom);
        };
    }
};

}
