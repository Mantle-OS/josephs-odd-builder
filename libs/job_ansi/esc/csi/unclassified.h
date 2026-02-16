#pragma once

#include <job_logger.h>

#include "job_ansi_screen.h"
#include "utils/job_ansi_enums.h"
#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {

class DispatchUnclassified : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchUnclassified(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override {
        // ICH - Insert Character(s)
        m_dispatchMap[CSI_CODE::ICH] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            int row = m_screen->cursor()->row();
            int col = m_screen->cursor()->column();
            int maxCol = m_screen->columnCount();

            if (col >= maxCol)
                return;

            // Move existing characters right to make space
            for (int i = maxCol - 1; i >= col + count; --i) {
                *m_screen->cellAt(row, i) = *m_screen->cellAt(row, i - count);
                m_screen->markCellDirty(row, i);
            }

            // Fill inserted space with blank cells
            for (int i = col; i < std::min(col + count, maxCol); ++i) {
                m_screen->cellAt(row, i)->reset();
                m_screen->markCellDirty(row, i);
            }
        };

        // NP - Next Page
        m_dispatchMap[CSI_CODE::NP] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            // Move cursor down by screen height * count
            int distance = m_screen->rowCount() * count;
            m_screen->moveCursorRelative(distance, 0, m_screen->originRow(), m_screen->originBottom());
        };

        // PP - Previous Page
        m_dispatchMap[CSI_CODE::PP] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            // Move cursor up by screen height * count
            int distance = m_screen->rowCount() * count;
            m_screen->moveCursorRelative(-distance, 0, m_screen->originRow(), m_screen->originBottom());
        };

        // DECLL - Load LEDs (ignored in modern terminals)
        m_dispatchMap[CSI_CODE::DECLL] = [](std::span<const int> params) {
            int mode = params.empty() ? 0 : params[0];
            JOB_LOG_DEBUG("[CSI:DECLL] Load LEDs mode: {}", mode);;
        };

        // DECSCUSR - Set Cursor Style
        m_dispatchMap[CSI_CODE::DECSCUSR] = [this](std::span<const int> params) {
            int value = params.empty() ? 0 : params[0];

            [[maybe_unused]]CursorStyle style;

            switch (value) {
            case 0:
            case 1:
                style = CursorStyle::BlinkingBlock;
                break;
            case 2:
                style = CursorStyle::SteadyBlock;
                break;
            case 3:
                style = CursorStyle::BlinkingUnderline;
                break;
            case 4:
                style = CursorStyle::SteadyUnderline;
                break;
            case 5:
                style = CursorStyle::BlinkingBar;
                break;
            case 6:
                style = CursorStyle::SteadyBar;
                break;
            default:
                JOB_LOG_DEBUG("[CSI:DECSCUSR] Unknown cursor style: {}", value);
                return;
            }
            // FIXME update this to use the enum and jsut static_cast the enum
            m_screen->setCursorStyle(value);
        };

        m_dispatchMap[CSI_CODE::UNKNOWN] = []([[maybe_unused]] std::span<const int> params) {
            JOB_LOG_DEBUG("[CSI:UNKNOWN] Unknown CSI sequence");
        };
    }
};

}
