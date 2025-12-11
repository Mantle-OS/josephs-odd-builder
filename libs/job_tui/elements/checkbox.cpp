#include "checkbox.h"
#include <algorithm>

namespace job::tui::gui{

Checkbox::Checkbox(JobTuiItem *parent) :
    JobTuiItem{parent}
{
    setItemHasContents(true);
    setFocusable(true);
    setAcceptsMouseEvents(true);
}

void Checkbox::setText(const std::string &text)
{
    m_text = text;
}

std::string Checkbox::text() const
{
    return m_text;
}

void Checkbox::setChecked(bool v)
{
    m_checked = v;
}

bool Checkbox::isChecked() const
{
    return m_checked;
}

void Checkbox::setEnabled(bool v)
{
    if (m_enabled == v)
        return;
    m_enabled = v;

    if (!m_enabled && focus())
        setFocus(false);
}

bool Checkbox::isEnabled() const
{
    return m_enabled;
}

void Checkbox::setNormalAttributes(ansi::Attributes::Ptr a)
{
    m_attrNormal = std::move(a);
}

void Checkbox::setFocusedAttributes(ansi::Attributes::Ptr a)
{
    m_attrFocused = std::move(a);
}

void Checkbox::setDisabledAttributes(ansi::Attributes::Ptr a)
{
    m_attrDisabled = std::move(a);
}

Checkbox::Visual Checkbox::visualState() const
{
    if (!m_enabled)
        return Visual::Disabled;

    if (focus())
        return Visual::Focused;

    return Visual::Normal;
}

void Checkbox::applyVisualAttributes(DrawContext &ctx) const
{
    auto choose = [&](Visual v) -> ansi::Attributes::Ptr {
        switch (v) {
        case Visual::Disabled:
            return m_attrDisabled ? m_attrDisabled : attributes();
        case Visual::Focused:
            return m_attrFocused  ? m_attrFocused  : attributes();
        case Visual::Normal:
        default:
            return m_attrNormal   ? m_attrNormal   : attributes();
        }
    };

    if (auto a = choose(visualState()))
        ctx.apply(*a);
}

void Checkbox::drawCheck(DrawContext &ctx, int x, int y) const
{
    ctx.moveCursor(x, y);
    m_checked ? ctx.write("[x]") :  ctx.write("[ ]");
}

void Checkbox::drawLabel(DrawContext &ctx, int x, int y, int w, int h) const {
    int lx = x + 4;                  // after "[ ] "
    int ly = y + std::max(0, (h-1)/2);
    ctx.moveCursor(lx, ly);
    // clamp if label would overflow
    int maxLabel = std::max(0, w - 4);
    std::string draw = m_text;

    if ((int)draw.size() > maxLabel && maxLabel > 3)
        draw = draw.substr(0, maxLabel - 3) + "...";
    else if ((int)draw.size() > maxLabel)
        draw = draw.substr(0, std::max(0, maxLabel));

    ctx.write(draw);
}

void Checkbox::paintSelf(DrawContext &ctx)
{
    if (!isVisible())
        return;

    const int x = this->x(), y = this->y(), w = this->width(), h = this->height();
    if (w <= 0 || h <= 0)
        return;

    // Fill background (optional; keeps consistent with Button theming)
    applyVisualAttributes(ctx);
    for (int dy = 0; dy < h; ++dy) {
        ctx.moveCursor(x, y + dy);
        ctx.write(std::string(w, ' '));
    }

    // Focus ring (like Button)
    if (focus())
        ctx.drawBox(x, y, w, h, DrawContext::BorderStyle::SINGLE);

    // Checkbox + label
    drawCheck(ctx, x + 1, y + std::max(0, (h - 1) / 2));
    drawLabel(ctx, x + 1, y, w - 2, h);

    ctx.resetColors();
}

bool Checkbox::focusEvent(const Event &event)
{
    if (!isEnabled() || !isVisible() || !isFocusable())
        return false;

    if (event.type() == Event::EventType::KeyPress) {
        const std::string k = event.key();
        if (k == " " || k == "\r" || k == "\n") {
            m_checked = !m_checked;
            if (onToggled) onToggled(m_checked);
            return true;
        }
        // Let app handle tab traversal
        if (k == "\t")
            return true;
    }

    return false;
}

// checkbox.cpp (replace mouseEvent with this)
bool Checkbox::mouseEvent(const Event &event)
{
    if (!isEnabled() || !isVisible())
        return false;

    const int ex = event.globalX();
    const int ey = event.globalY();

    const int x = this->x();
    const int y = this->y();
    const int w = this->width();
    const int h = this->height();

    const bool inside = (ex >= x && ex < x + w && ey >= y && ey < y + h);

    switch (event.type()) {
    case Event::EventType::MousePress:
        if (inside) {
            m_pressed = true;
            if (!focus())
                setFocus(true);  // grab focus on press

            return true;
        }
        m_pressed = false;
        return false;

    case Event::EventType::MouseRelease:
        if (m_pressed && inside) {
            m_pressed = false;
            m_checked = !m_checked;
            if (onToggled)
                onToggled(m_checked);  // make sure caller marks dirty

            return true;
        }
        m_pressed = false;
        return false;

    default:
        return false;
    }
}
}
