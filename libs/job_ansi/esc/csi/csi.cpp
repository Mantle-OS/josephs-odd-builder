#include "csi.h"

namespace job::ansi::csi {

DispatchCSI::DispatchCSI(Screen *screen) :
    m_screen(screen),
    m_SGR(screen),
    m_DECSET(screen),
    m_charAttr(screen),
    m_charWidth(screen),
    m_cursor(screen),
    m_deviceStatus(screen),
    m_rectOps(screen),
    m_scroll(screen),
    m_tabulation(screen),
    m_unclassified(screen),
    m_windowOps(screen),
    m_xtermExt(screen)
{
}

void DispatchCSI::dispatch(CSI_CODE code, std::span<const int> params)
{
    switch (code) {
    // Cursor
    case CSI_CODE::CUU:
    case CSI_CODE::CUD:
    case CSI_CODE::CUF:
    case CSI_CODE::CUB:
    case CSI_CODE::CNL:
    case CSI_CODE::CPL:
    case CSI_CODE::CHA:
    case CSI_CODE::HPR:
    case CSI_CODE::CUP:
    case CSI_CODE::VPA:
    case CSI_CODE::HVP:
    case CSI_CODE::SCP:
    case CSI_CODE::RCP:
        m_cursor.dispatch(code, params);
        break;

    // Scroll
    case CSI_CODE::IL:
    case CSI_CODE::DL:
    case CSI_CODE::SU:
    case CSI_CODE::SD:
    case CSI_CODE::SR:
    case CSI_CODE::DECSTBM:
        m_scroll.dispatch(code, params);
        break;

    // CharAttr
    case CSI_CODE::SGR: {
        m_SGR.dispatch(params);
        break;
    }
    case CSI_CODE::SEE:
    case CSI_CODE::DECSCA:
    case CSI_CODE::ED:
    case CSI_CODE::EL:
    case CSI_CODE::EF:
    case CSI_CODE::EA:
    case CSI_CODE::DCH:
    case CSI_CODE::ECH:
    case CSI_CODE::REP:
        m_charAttr.dispatch(code, params);
        break;

    // Tabulation
    case CSI_CODE::CHT:
    case CSI_CODE::CBT:
    case CSI_CODE::TBC:
    case CSI_CODE::CTC:
        m_tabulation.dispatch(code, params);
        break;

    // DeviceStatus
    case CSI_CODE::DA:
    case CSI_CODE::DA2:
    case CSI_CODE::DA3:
    case CSI_CODE::DSR:
    case CSI_CODE::CPR:
    case CSI_CODE::MC:
        m_deviceStatus.dispatch(code, params);
        break;

    // ModeChange (handled by DispatchDECSET directly)
    case CSI_CODE::SM: {
        for (int p : params) {
            if (m_privateMode)
                m_DECSET.dispatchPrivate(static_cast<DECSET_PRIVATE_CODE>(p), true);
            else
                m_DECSET.dispatchPublic(static_cast<DECSET_PUBLIC_CODE>(p), true);
        }
        break;
    }
    case CSI_CODE::RM: {
        for (int p : params) {
            if (m_privateMode)
                m_DECSET.dispatchPrivate(static_cast<DECSET_PRIVATE_CODE>(p), false);
            else
                m_DECSET.dispatchPublic(static_cast<DECSET_PUBLIC_CODE>(p), false);
        }
        break;
    }

    // RectOps
    case CSI_CODE::DECCRA:
    case CSI_CODE::DECFRA:
    case CSI_CODE::DECRARA:
        m_rectOps.dispatch(code, params);
        break;

    // WindowOps
    case CSI_CODE::WINDOW_MANIP:
        m_windowOps.dispatch(code, params);
        break;

    // CharWidth
    case CSI_CODE::DECSWL:
    case CSI_CODE::DECDWL:
    case CSI_CODE::DECDHL_TOP:
    case CSI_CODE::DECDHL_BOTTOM:
        m_charWidth.dispatch(code, params);
        break;

    // XtermExt
    case CSI_CODE::PASTE_END:
        m_xtermExt.dispatch(code, params);
        break;

    // Unclassified
    case CSI_CODE::ICH:
    case CSI_CODE::NP:
    case CSI_CODE::PP:
    case CSI_CODE::DECLL:
    case CSI_CODE::DECSCUSR:
    case CSI_CODE::UNKNOWN:
        m_unclassified.dispatch(code, params);
        break;

    case CSI_CODE::TILDE:
        dispatchTilde(params);
        break;

    default:
        JOB_LOG_DEBUG("Unhandled CSI code: {}", static_cast<int>(code));
        break;
    }

    // Always reset private mode after dispatch
    m_privateMode = false;
}

void DispatchCSI::setLastPrintedChar(char32_t ch)
{
    m_lastPrintedChar = ch;
}

void DispatchCSI::setLastPrintedChar(std::optional<char32_t> ch)
{
    m_lastPrintedChar = ch;
}

void DispatchCSI::clearLastPrintedChar()
{
    m_lastPrintedChar.reset();
}

void DispatchCSI::handlePrintable(char32_t ch)
{
    if (m_screen) {
        setLastPrintedChar(ch);
        m_screen->putChar(ch);
    }
}

int DispatchCSI::getParam(std::span<const int> params, size_t index, int def)
{
    if (index < params.size())
        return params[index];
    return def;
}

bool DispatchCSI::privateMode() const
{
    return m_privateMode;
}

void DispatchCSI::setPrivateMode(bool privateMode)
{
    m_privateMode = privateMode;
}

void job::ansi::csi::DispatchCSI::dispatchTilde(std::span<const int> params)
{
    if (params.empty()) {
        JOB_LOG_DEBUG("[CSI ~] No parameter provided");
        return;
    }
    auto mode = static_cast<TILDE_MODE>(params[0]);
    switch (mode) {
    case TILDE_MODE::BRACKETED_PASTE_START_CODE:
        if (m_screen->get_bracketedPasteEnabled()) {
            // this is how we alert the gui frontend that bracketedPasteEnabled Changed..
            m_screen->set_bracketedPasteEnabled(true);
        }
        break;
    case TILDE_MODE::BRACKETED_PASTE_END_CODE:
        if (m_screen->get_bracketedPasteEnabled()) {
            m_screen->set_bracketedPasteEnabled(false);
        }
        break;
    default:
        JOB_LOG_DEBUG("[CSI ~] Unknown tilde param: {}",  static_cast<int>(mode));
        break;
    }
}

}

