#pragma once

#include <optional>

#include "utils/job_ansi_enums.h"
#include "job_ansi_screen.h"
#include "esc/csi/sgr.h"
#include "esc/csi/mode_change.h"
#include "esc/csi/char_attr.h"
#include "esc/csi/char_width.h"
#include "esc/csi/cursor.h"
#include "esc/csi/device_status.h"
#include "esc/csi/rect_ops.h"
#include "esc/csi/scroll.h"
#include "esc/csi/tabulation.h"
#include "esc/csi/unclassified.h"
#include "esc/csi/window_ops.h"
#include "esc/csi/xterm_ext.h"

namespace job::ansi::csi {
class DispatchCSI {
public:

    explicit DispatchCSI(Screen *screen);
    void dispatch(CSI_CODE code, std::span<const int> params);
    void setLastPrintedChar(char32_t ch);
    void setLastPrintedChar(std::optional<char32_t> ch);
    void clearLastPrintedChar();
    void handlePrintable(char32_t ch);
    bool privateMode() const;
    void setPrivateMode(bool privateMode);
    void dispatchTilde(std::span<const int> params);

private:
    Screen                  *m_screen    = nullptr;
    DispatchSGR             m_SGR;
    DispatchDECSET          m_DECSET;
    DispatchCharAttr        m_charAttr;
    DispatchCharWidth       m_charWidth;
    DispatchCursor          m_cursor;
    DispatchDeviceStatus    m_deviceStatus;
    DispatchRectOps         m_rectOps;
    DispatchScroll          m_scroll;
    DispatchTabulation      m_tabulation;
    DispatchUnclassified    m_unclassified;
    DispatchWindowOps       m_windowOps;
    DispatchXtermExt        m_xtermExt;
    std::optional<char32_t> m_lastPrintedChar;
    bool                    m_privateMode = false;


    static int getParam(std::span<const int> params, size_t index, int def = 0);
};

}
