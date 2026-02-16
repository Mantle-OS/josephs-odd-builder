#pragma once


#include <sstream>

#include <job_logger.h>

#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"
#include "esc/csi/dispatch_base.h"

namespace job::ansi::csi {
class DispatchDeviceStatus : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchDeviceStatus(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        // DSR - Device Status Report
        m_dispatchMap[CSI_CODE::DSR] = [this](std::span<const int> params) {
            DSR_MODE mode = static_cast<DSR_MODE>(params.empty() ? 0 : params[0]);
            
            switch (mode) {
            case DSR_MODE::DSR_STATUS:
                // Report "OK" status
                m_screen->set_cursor_report("\033[0n");
                break;
                
                /////// FIXME
            case DSR_MODE::DSR_CURSOR: {
                const auto *cursor = m_screen->cursor();
                std::ostringstream oss;
                oss << "\033[" << (cursor->row() + 1) << ";" << (cursor->column() + 1) << "R";
                m_screen->set_cursor_position_report(oss.str());
                break;
            }
                
            case DSR_MODE::DSR_PRINTER_STATUS:
                // Report no printer
                m_screen->set_cursor_report("\033[?13n");
                break;
                
            case DSR_MODE::DSR_UDK_STATUS:
                // Report UDK locked
                m_screen->set_cursor_report("\033[?21n");
                break;
                
            case DSR_MODE::DSR_KEYBOARD_LANG:
                // Report English keyboard
                m_screen->set_cursor_report("\033[?26;1n");
                break;
                
            default:
                JOB_LOG_DEBUG("[CSI:DSR] Unknown status report mode: {}", static_cast<int>(mode));
                break;
            }
        };

        // DA - Primary Device Attributes
        m_dispatchMap[CSI_CODE::DA] = [this](std::span<const int> params) {
            if (params.empty() || params[0] == 0) {
                // Report VT100 with Advanced Video Option (AVO)
                m_screen->set_device_attr(m_screen->get_device_attr());
            } else {
                JOB_LOG_DEBUG("[CSI:DA] Unknown device attributes query: {}", params[0]);
            }
        };

        // DA2 - Secondary Device Attributes
        m_dispatchMap[CSI_CODE::DA2] = [this](std::span<const int> params) {
            if (params.empty() || params[0] == 0) {
                // Report terminal version
                m_screen->set_device_attr_secondary(m_screen->get_device_attr_secondary());
            } else {
                JOB_LOG_DEBUG("[CSI:DA2] Unknown secondary DA query: {}", params[0]);
            }
        };

        // DA3 - Tertiary Device Attributes
        m_dispatchMap[CSI_CODE::DA3] = [this](std::span<const int> params) {
            if (params.empty() || params[0] == 0) {
                // Report terminal unit ID
                m_screen->set_device_attr_tertiary(m_screen->get_device_attr_tertiary());
            } else {
                JOB_LOG_DEBUG("[CSI:DA3] Unknown tertiary DA query: {}", params[0]);
            }
        };
    }
};

}
