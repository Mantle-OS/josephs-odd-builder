#pragma once

#include <job_logger.h>

#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"
#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {

using namespace job::ansi::utils;

class DispatchCharAttr final : public DispatchBase<ansi::utils::CSI_CODE> {
public:
    explicit DispatchCharAttr(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        m_dispatchMap[CSI_CODE::ED] = [this](std::span<const int> params) {
            ED_MODE mode = static_cast<ED_MODE>(params.empty() ? 0 : params[0]);
            if (Cursor *cursor = m_screen->cursor()) {
                switch (mode) {
                case ED_MODE::TO_END:
                    m_screen->eraseToEnd(cursor->row(), cursor->column());
                    break;
                case ED_MODE::TO_START:
                    m_screen->eraseToStart(cursor->row(), cursor->column());
                    break;
                case ED_MODE::ENTIRE_SCREEN:
                    m_screen->eraseScreen();

                    break;
                case ED_MODE::SCREEN_AND_SCROLLBACK:
                    m_screen->eraseScreenAndScrollback();
                    break;
                }
            }
        };

        m_dispatchMap[CSI_CODE::EL] = [this](std::span<const int> params) {
            EL_MODE mode = static_cast<EL_MODE>(params.empty() ? 0 : params[0]);
            if (Cursor *cursor = m_screen->cursor()) {
                switch (mode) {
                case EL_MODE::TO_END:
                    m_screen->eraseLineToEnd(cursor->row(), cursor->column());
                    break;
                case EL_MODE::TO_START:
                    m_screen->eraseLineToStart(cursor->row(), cursor->column());
                    break;
                case EL_MODE::ENTIRE_LINE:
                    m_screen->eraseLine(cursor->row());
                    break;
                }
            }
        };

        m_dispatchMap[CSI_CODE::DCH] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->deleteChars(count);
        };

        m_dispatchMap[CSI_CODE::ECH] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->eraseChars(count);
        };

        m_dispatchMap[CSI_CODE::REP] = [this](std::span<const int> params) {
            int count = params.empty() ? 1 : params[0];
            m_screen->repeatLastChar(count);
        };

        // SEE - Select Editing Extent
        m_dispatchMap[CSI_CODE::SEE] = [this](std::span<const int> params) {
            SEE_MODE mode = static_cast<SEE_MODE>(params.empty() ? 0 : params[0]);
            JOB_LOG_DEBUG("[CSI:SEE] Select Editing Extent mode: {}", static_cast<int>(mode));;
            m_screen->setEditingExtent(mode);
        };

        // DECSCA - Select Character Protection Attribute
        m_dispatchMap[CSI_CODE::DECSCA] = [this](std::span<const int> params) {
            DECSCA_MODE mode = static_cast<DECSCA_MODE>(params.empty() ? 0 : params[0]);
            JOB_LOG_DEBUG("[CSI:DECSCA] Set character protection mode: {}",static_cast<int>(mode));
            m_screen->setProtectionMode(mode);
        };

        // EF - Erase Field (VT220)
        m_dispatchMap[CSI_CODE::EF] = [this](std::span<const int> params) {
            int mode = params.empty() ? 0 : params[0];
            EF_MODE efMode = static_cast<EF_MODE>(mode);

            if (Cursor *cursor = m_screen->cursor()) {
                m_screen->eraseField(cursor->row(), cursor->column(), efMode);
            }
        };

        // EA - Erase Area (VT220)
        m_dispatchMap[CSI_CODE::EA] = [this](std::span<const int> params) {
            int mode = params.empty() ? 0 : params[0];
            EA_MODE eaMode = static_cast<EA_MODE>(mode);
            m_screen->eraseArea(eaMode);
        };

    }
};
}

