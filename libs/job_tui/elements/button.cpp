#include "button.h"
#include <algorithm>

namespace job::tui::gui {
using job::tui::Event;

Button::Button(JobTuiItem *parent) :
    JobTuiItem{parent}
{
    setItemHasContents(true);
    setFocusable(true);
    setAcceptsMouseEvents(true);
}

void Button::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    if (!m_enabled && focus())
        setFocus(false);
}

bool Button::isEnabled() const
{
    return m_enabled;
}

void Button::setText(const std::string &text)
{
    m_text = text;
}

std::string Button::text() const
{
    return m_text;
}

void Button::setNormalAttributes(ansi::Attributes::Ptr a)
{
    m_attrNormal = std::move(a);
}

void Button::setFocusedAttributes(ansi::Attributes::Ptr a)
{
    m_attrFocused = std::move(a);
}

void Button::setPressedAttributes(ansi::Attributes::Ptr a)
{
    m_attrPressed = std::move(a);
}

void Button::setDisabledAttributes(ansi::Attributes::Ptr a)
{
    m_attrDisabled = std::move(a);
}

Button::Visual Button::visualState() const
{
    if (!m_enabled)
        return Visual::Disabled;

    if (m_pressed)
        return Visual::Pressed;

    if (focus())
        return Visual::Focused;

    return Visual::Normal;
}

void Button::applyVisualAttributes(DrawContext &ctx) const
{
    auto choose = [&](Visual v) -> ansi::Attributes::Ptr {
        switch (v) {
        case Visual::Disabled:
            return m_attrDisabled ? m_attrDisabled : attributes();
        case Visual::Pressed:
            return m_attrPressed ? m_attrPressed  : attributes();
        case Visual::Focused:
            return m_attrFocused ? m_attrFocused  : attributes();
        case Visual::Normal:
        default:
            return m_attrNormal ? m_attrNormal   : attributes();
        }
    };

    auto a = choose(visualState());
    if (a)
        ctx.apply(*a);
}

void Button::drawLabel(DrawContext &ctx, int x, int y, int w, int h) const
{
    int labelLen = static_cast<int>(m_text.size());
    int lx = x + std::max(0, (w - labelLen) / 2);
    int ly = y + std::max(0, (h - 1) / 2);
    ctx.moveCursor(lx, ly);
    ctx.write(m_text);
}

void Button::paintSelf(DrawContext &ctx)
{
    if (!isVisible())
        return;

    int x = this->x();
    int y = this->y();
    int w = this->width();
    int h = this->height();

    if (w <= 0 || h <= 0)
        return;

    // Fill
    applyVisualAttributes(ctx);
    for (int dy = 0; dy < h; ++dy) {
        ctx.moveCursor(x, y + dy);
        ctx.write(std::string(w, ' '));
    }

    // Focus ring / border
    if (focus())
        ctx.drawBox(x, y, w, h, DrawContext::BorderStyle::SINGLE);

    // Label
    drawLabel(ctx, x, y, w, h);

    ctx.resetColors();
}

bool Button::focusEvent(const Event &event)
{
    if (!isEnabled() || !isVisible() || !isFocusable())
        return false;

    if (event.type() == Event::EventType::KeyPress){
        const std::string k = event.key();
        if (k == " " || k == "\r" || k == "\n") {
            // Click on Enter/Space
            if (onClicked) onClicked();
            return true;
        }
        if (k == "\t") {
            // Let app handle Tab traversal
            return true;
        }
    }
    return false;
}

bool Button::mouseEvent(const Event &event)
{
    if (!isEnabled() || !isVisible())
        return false;

    const int ex = event.globalX();
    const int ey = event.globalY();

    const int x = this->x();
    const int y = this->y();
    const int w = this->width();
    const int h = this->height();

    if (ex >= x && ex < x + w && ey >= y && ey < y + h) {
        switch (event.type()) {
        case Event::EventType::MousePress:
            m_pressed = true;
            return true;
        case Event::EventType::MouseRelease:
            if (m_pressed && onClicked) onClicked();
            m_pressed = false;
            return true;
        default: break;
        }
    } else if (event.type() == Event::EventType::MouseRelease) {
        m_pressed = false;
    }
    return false;
}

}
