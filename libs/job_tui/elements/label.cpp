#include "label.h"
#include <cstring>

#include "job_tui_draw_context.h"

static std::string elidedText(const std::string &text, int maxWidth)
{
    if (int(text.size()) <= maxWidth)
        return text;

    if (maxWidth <= 3)
        return std::string(maxWidth, '.');

    return text.substr(0, maxWidth - 3) + "...";
}

static std::vector<std::string> wordWrap(const std::string &text, int maxWidth)
{
    std::vector<std::string> lines;
    std::string current;
    std::istringstream stream(text);
    std::string word;

    while (stream >> word) {
        if (!current.empty() && int(current.size() + word.size() + 1) > maxWidth) {
            lines.push_back(current);
            current = word;
        } else {
            if (!current.empty())
                current += ' ';
            current += word;
        }
    }

    if (!current.empty())
        lines.push_back(current);

    return lines;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace job::tui::gui {

Label::Label(JobTuiItem *parent):
    JobTuiItem{parent}
{
    setItemHasContents(true);
}

Label::WrapMode Label::wrapMode() const
{
    return m_wrapMode;
}

Label::ElideMode Label::elideMode() const
{
    return m_elideMode;
}

Label::TextAlignment Label::verticalAlignment() const
{
    return m_verticalAlignment;
}

Label::TextAlignment Label::horizontalAlignment() const
{
    return m_horizontalAlignment;
}

void Label::setText(const std::string &text)
{
    m_text = text;
}

std::string Label::text() const
{
    return m_text;
}

void Label::setHorizontalAlignment(TextAlignment a)
{
    m_horizontalAlignment = a;
}

void Label::setVerticalAlignment(TextAlignment a)
{
    m_verticalAlignment = a;
}

void Label::setElideMode(ElideMode mode)
{
    m_elideMode = mode;
}

void Label::setWrapMode(WrapMode mode)
{
    m_wrapMode = mode;
}

void Label::setForeground(const ansi::RGBColor &color)
{
    attributes()->setForeground(color);
}

void Label::setBackground(const ansi::RGBColor &color)
{
    attributes()->setBackground(color);
}

void Label::setUnderline(ansi::utils::enums::UnderlineStyle style)
{
    attributes()->setUnderline(style);
}

void Label::setBold(bool on)
{
    attributes()->bold = on;
}

void Label::setItalic(bool italic)
{
    attributes()->italic = italic;
}

void Label::setStrikethrough(bool on)
{
    attributes()->strikethrough = on;
}

void Label::paintSelf(DrawContext &ctx)
{
    if (!isVisible())
        return;

    auto attr = attributes();
    if (!attr)
        return;

    const int contentWidth  = width();
    const int contentHeight = height();

    std::vector<std::string> lines;

    // Wrap or elide text based on settings
    switch (m_wrapMode) {
    case WrapMode::NoWrap: {
        std::string line = m_text;
        if (int(line.size()) > contentWidth)
            line = elidedText(line, contentWidth);

        lines.push_back(line);
        break;
    }
    case WrapMode::CharWrap: {
        for (size_t i = 0; i < m_text.size(); i += contentWidth)
            lines.push_back(m_text.substr(i, contentWidth));
        break;
    }
    case WrapMode::WordWrap: {
        lines = wordWrap(m_text, contentWidth);
        break;
    }
    }

    // Clamp to available height
    if (int(lines.size()) > contentHeight)
        lines.resize(contentHeight);

    // Compute vertical offset based on alignment
    int yOffset = 0;
    if (m_verticalAlignment == TextAlignment::AlignVCenter)
        yOffset = (contentHeight - int(lines.size())) / 2;
    else if (m_verticalAlignment == TextAlignment::AlignBottom)
        yOffset = contentHeight - int(lines.size());

    // Draw each line with proper horizontal alignment and attribute application
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string &line = lines[i];

        int xOffset = 0;
        if (m_horizontalAlignment == TextAlignment::AlignHCenter)
            xOffset = (contentWidth - int(line.size())) / 2;
        else if (m_horizontalAlignment == TextAlignment::AlignRight)
            xOffset = contentWidth - int(line.size());

        ctx.moveCursor(x() + xOffset, y() + yOffset + int(i));
        ctx.resetColors();           // Prevent SGR leakage
        ctx.apply(*attr);            // Re-apply style
        ctx.write(line);
    }
}



} 
