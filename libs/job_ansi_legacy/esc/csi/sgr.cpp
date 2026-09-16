#include "sgr.h"

#include <job_ansi_attributes.h>
#include <rgb_color.h>
#include <job_ansi_utils.h>
#include "job_ansi_enums.h"
#include "color_utils.h"
namespace job::ansi::csi {

DispatchSGR::DispatchSGR(Screen *screen) :
    m_screen(screen)
{

}

void DispatchSGR::dispatch(std::span<const int> params)
{

    if (!m_screen)
        return;

    Attributes *attributes = m_screen->currentAttributes();
    if (!attributes)
        return;

    size_t i = 0;
    while (i < params.size()) {
        int raw = params[i];
        auto code = static_cast<SGR_CODE>(raw);

        switch (code) {
        case SGR_CODE::RESET:
            attributes->reset();
            break;
        case SGR_CODE::BOLD:
            attributes->bold = true;
            break;
        case SGR_CODE::FAINT:
            attributes->faint = true;
            break;
        case SGR_CODE::ITALIC:
            attributes->italic = true;
            break;
        case SGR_CODE::UNDERLINE:
            attributes->setUnderline(UnderlineStyle::Single);
            break;
        case SGR_CODE::DOUBLY_UNDERLINE:
            attributes->setUnderline(UnderlineStyle::Double);
            break;
        case SGR_CODE::NO_UNDERLINE:
            attributes->setUnderline(UnderlineStyle::None);
            break;
        case SGR_CODE::SLOW_BLINK:
            attributes->blink = 1;
            break;
        case SGR_CODE::RAPID_BLINK:
            attributes->blink = 2;
            break;
        case SGR_CODE::NO_BLINK:
            attributes->blink = 0;
            break;
        case SGR_CODE::INVERSE:
            attributes->inverse = true;
            break;
        case SGR_CODE::NO_INVERSE:
            attributes->inverse = false;
            break;
        case SGR_CODE::STRIKETHROUGH:
            attributes->strikethrough = true;
            break;
        case SGR_CODE::NO_STRIKETHROUGH:
            attributes->strikethrough = false;
            break;
        case SGR_CODE::CONCEAL:
            attributes->conceal = true;
            break;
        case SGR_CODE::NO_CONCEAL:
            attributes->conceal = false;
            break;
        case SGR_CODE::FG_DEFAULT:
            attributes->resetForeground();
            break;
        case SGR_CODE::BG_DEFAULT:
            attributes->resetBackground();
            break;
        case SGR_CODE::EXTENDED_FG: {
            if (i + 1 >= params.size()) break;
            int mode = params[i + 1];

            if (mode == 5 && i + 2 < params.size()) {
                attributes->setForeground(ansi::utils::fromXterm256Palette(params[i + 2]));
                i += 2;
            } else if (mode == 2 && i + 4 < params.size()) {
                attributes->setForeground(RGBColor{
                    static_cast<uint8_t>(params[i + 2]),
                    static_cast<uint8_t>(params[i + 3]),
                    static_cast<uint8_t>(params[i + 4])
                });
                i += 4;
            }
            break;
        }

        case SGR_CODE::EXTENDED_BG: {
            if (i + 1 >= params.size()) break;
            int mode = params[i + 1];

            if (mode == 5 && i + 2 < params.size()) {
                attributes->setBackground(ansi::utils::fromXterm256Palette(params[i + 2]));
                i += 2;
            } else if (mode == 2 && i + 4 < params.size()) {
                attributes->setBackground(RGBColor{
                    static_cast<uint8_t>(params[i + 2]),
                    static_cast<uint8_t>(params[i + 3]),
                    static_cast<uint8_t>(params[i + 4])
                });
                i += 4;
            }
            break;
        }

        case SGR_CODE::FRAME:
            attributes->framed = true;
            break;
        case SGR_CODE::ENCIRCLED:
            attributes->encircled = true;
            break;
        case SGR_CODE::OVERLINE:
            attributes->overline = true;
            break;
        case SGR_CODE::NO_FRAME_NO_CIRCLE:
            attributes->framed = false;
            attributes->encircled = false;
            break;
        case SGR_CODE::NO_OVERLINE:
            attributes->overline = false;
            break;
        default: {
            if (auto parsed = ansi::utils::sgrCodeToColor(raw)) {
                if (parsed->isForeground)
                    attributes->setForeground(parsed->color);
                else
                    attributes->setBackground(parsed->color);
            }
            break;
        }
        }

        ++i;
    }

    // Intern attributes and update screen
    auto interned = Attributes::intern(*attributes);
    m_screen->setCurrentAttributes(interned);
}

}

