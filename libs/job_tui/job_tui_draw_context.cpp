#include "job_tui_draw_context.h"

#include <color_utils.h>
#include <job_ansi_suffix.h>
#include <job_ansi_attributes.h>

namespace job::tui {

using namespace job::ansi::utils;
using namespace job::ansi::utils::suffix;

DrawContext::DrawContext(job::core::IODevice::Ptr device) :
    m_device(std::move(device))
{
    m_fg = RGBColor(255, 255, 255);
    m_bg = RGBColor(0, 0, 0);
    m_writeOk = true;
}

void DrawContext::write(std::string_view data)
{
    if (!m_device || m_device->write(std::string(data)) < 0)
        m_writeOk = false;
}

bool DrawContext::ok() const
{
    return m_writeOk;
}

void DrawContext::setColor(const RGBColor &fg, const RGBColor &bg)
{
    setForeground(fg);
    setBackground(bg);
}

void DrawContext::setForeground(const RGBColor &fg)
{
    m_fg = fg;
    applyColor(m_fg, true);
}

void DrawContext::setBackground(const RGBColor &bg)
{
    m_bg = bg;
    applyColor(m_bg, false);
}

void DrawContext::moveCursor(int x, int y)
{
    std::string seq = CSI + std::to_string(y) + ";" + std::to_string(x) + "H";
    write(seq);
}

void DrawContext::resetColors()
{
    m_fg = RGBColor{255, 255, 255}; // or default terminal fg
    m_bg = RGBColor{0, 0, 0};       // or default terminal bg

    m_writeOk = true;

    write(CSI + std::string("0") + SGR_SUFFIX);
}

void DrawContext::applyColor(const RGBColor &color, bool isForeground)
{
    std::optional<int> sgr = rgbToSGR(color, isForeground);

    if (sgr)
        write(CSI + std::to_string(*sgr) + SGR_SUFFIX);
    else
        write(sgrTrueColor(color, isForeground));
}

void DrawContext::apply(const ansi::Attributes &attr)
{
    std::vector<std::string> sgrCodes;

    // Text styles
    if (attr.bold)
        sgrCodes.emplace_back("1");

    if (attr.faint)
        sgrCodes.emplace_back("2");

    if (attr.italic)
        sgrCodes.emplace_back("3");

    // Underline styles: 4=single, 21=double, 4:3=dotted, 4:4=dashed, 4:5=curly
    switch (attr.getUnderline()) {
    case ansi::UnderlineStyle::Single:
        sgrCodes.emplace_back("4");
        break;
    case ansi::UnderlineStyle::Double:
        sgrCodes.emplace_back("21");
        break;
    case ansi::UnderlineStyle::Curly:
        sgrCodes.emplace_back("4:5");
        break;
    case ansi::UnderlineStyle::Dotted:
        sgrCodes.emplace_back("4:3");
        break;
    case ansi::UnderlineStyle::Dashed:
        sgrCodes.emplace_back("4:4");
        break;
    default: break;
    }

    // Blink modes
    if (attr.blink == 1)
        sgrCodes.emplace_back("5");
    else if (attr.blink == 2)
        sgrCodes.emplace_back("6");

    if (attr.inverse)
        sgrCodes.emplace_back("7");

    if (attr.conceal)
        sgrCodes.emplace_back("8");

    if (attr.strikethrough)
        sgrCodes.emplace_back("9");

    // Box styles
    if (attr.framed)
        sgrCodes.emplace_back("51");

    if (attr.encircled)
        sgrCodes.emplace_back("52");

    if (attr.overline)
        sgrCodes.emplace_back("53");

    // Compose and emit full SGR
    if (!sgrCodes.empty()) {
        std::string sequence = CSI;
        sequence += sgrCodes[0];
        for (size_t i = 1; i < sgrCodes.size(); ++i) {
            sequence += ';';
            sequence += sgrCodes[i];
        }
        sequence += SGR_SUFFIX;
        write(sequence);
    }

    // Apply 24-bit foreground/background
    if (auto *fg = attr.getForeground())
        setForeground(*fg);

    if (auto *bg = attr.getBackground())
        setBackground(*bg);
}

void DrawContext::drawBox(int width, int height)
{
    drawBox(0, 0, width, height, BorderStyle::SINGLE);
}

void DrawContext::drawBox(Rect rect, BorderStyle style)
{
    drawBox(rect.x, rect.y, rect.width, rect.height, style);
}

void DrawContext::drawBox(int x, int y, int width, int height, BorderStyle style)
{

    if (width < 2 || height < 2)
        return;

    const char *tl, *tr, *bl, *br, *h, *v;

    switch (style) {
    case BorderStyle::SINGLE:
        tl = "┌"; tr = "┐"; bl = "└"; br = "┘"; h = "─"; v = "│";
        break;
    case BorderStyle::DOUBLE:
        tl = "╔"; tr = "╗"; bl = "╚"; br = "╝"; h = "═"; v = "║";
        break;
    case BorderStyle::LINE:
        tl = "+"; tr = "+"; bl = "+"; br = "+"; h = "-"; v = "|";
        break;
    case BorderStyle::DOT:
        tl = "*"; tr = "*"; bl = "*"; br = "*"; h = "."; v = ":";
        break;
    case BorderStyle::BLOCK:
        tl = "█"; tr = "█"; bl = "█"; br = "█"; h = "█"; v = "█";
        break;
    case BorderStyle::DarkShade:
        tl = "▓"; tr = "▓"; bl = "▓"; br = "▓"; h = "▓"; v = "▓";
        break;
    case BorderStyle::MediumShade:
        tl = "▒"; tr = "▒"; bl = "▒"; br = "▒"; h = "▒"; v = "▒";
        break;
    case BorderStyle::LightShade:
        tl = "░"; tr = "░"; bl = "░"; br = "░"; h = "░"; v = "░";
        break;
    }

    // Top
    moveCursor(x, y);
    write(tl);

    for (int i = 0; i < width - 2; ++i)
        write(h);

    write(tr);

    // Sides
    for (int i = 1; i < height - 1; ++i) {
        moveCursor(x, y + i);
        write(v);
        moveCursor(x + width - 1, y + i);
        write(v);
    }

    // Bottom
    moveCursor(x, y + height - 1);
    write(bl);
    for (int i = 0; i < width - 2; ++i)
        write(h);

    write(br);
}

}
