#include "rectangle.h"
#include "color_utils.h"

namespace job::tui::gui {

Rectangle::Rectangle(JobTuiItem *parent) :
    JobTuiItem{parent}
{
    setItemHasContents(true);

    // Defaults
    ansi::RGBColor bgColor{0, 0, 0};
    ansi::RGBColor fgColor{255, 255, 255};

    // Access via the new pointer-based API
    if (auto attr = attributes()) {
        if (auto *bg = attr->getBackground()) {
            bgColor = *bg;
        }
        if (auto *fg = attr->getForeground()) {
            fgColor = *fg;
        }
    }

    m_color = bgColor.toHexString();
    m_borderColor = fgColor.toHexString();
}


void Rectangle::paintSelf(DrawContext &ctx)
{
    if (!isVisible())
        return;

    // Rect rec = boundingRect();
    const int x = this->x();
    const int y = this->y();
    const int w = this->width();
    const int h = this->height();
    bool isHorizontal = (m_gradient && m_gradient->orientation() == GradientOrientation::HORIZONTAL);

    if (isHorizontal) {
        for (int row = 0; row < h; ++row) {
            ctx.moveCursor(x, y + row);

            for (int col = 0; col < w; ++col) {
                auto attr = updateAttributesForRow(col, w);
                if (attr) ctx.apply(*attr);
                ctx.write(" ");
            }
        }
    } else {
        for (int row = 0; row < h; ++row) {
            ctx.moveCursor(x, y + row);
            auto attr = updateAttributesForRow(row, h);
            if (attr) ctx.apply(*attr);
            ctx.write(std::string(w, ' ')); // Write full line
        }
    }

    ctx.resetColors();

    if (m_hasBorder && m_borderWidth > 0 && m_borderWidth == 1) {
        ctx.setForeground(resolveColor(m_borderColor).value_or(ansi::RGBColor{255, 255, 255}));
        ctx.drawBox(x, y, w, h, static_cast<DrawContext::BorderStyle>(m_borderStyle));
    }
}

std::string Rectangle::color() const
{
    return m_color;
}

void Rectangle::setColor(const std::string &color)
{
    m_color = color;
}

void Rectangle::setColor(ansi::utils::RGBColor color)
{
    setColor(color.toHexString());
}

Gradient *Rectangle::gradient() const
{
    return m_gradient.get();
}

void Rectangle::setGradient(std::unique_ptr<Gradient> gradient)
{
    m_gradient = std::move(gradient);
}

uint8_t Rectangle::radius() const
{
    return m_radius;
}

void Rectangle::setRadius(uint8_t radius)
{
    m_radius = radius;
}

uint8_t Rectangle::opacity() const
{
    return m_opacity;
}

void Rectangle::setOpacity(uint8_t opacity)
{
    m_opacity = opacity;
}

bool Rectangle::hasBorder() const
{
    return m_hasBorder;
}

void Rectangle::setHasBorder(bool hasBorder)
{
    m_hasBorder = hasBorder;
}

std::string Rectangle::borderColor() const
{
    return m_borderColor;
}

void Rectangle::setBorderColor(const std::string &borderColor)
{
    m_borderColor = borderColor;
}

void Rectangle::setBorderColor(ansi::utils::RGBColor color)
{
    m_borderColor = color.toHexString();
}

uint8_t Rectangle::borderWidth() const
{
    return m_borderWidth;
}

void Rectangle::setBorderWidth(uint8_t borderWidth)
{
    m_borderWidth = borderWidth;
}

DrawContext::BorderStyle Rectangle::borderStyle() const
{
    return m_borderStyle;
}

void Rectangle::setBorderStyle(DrawContext::BorderStyle style)
{
    m_borderStyle = style;
}

std::optional<ansi::RGBColor> Rectangle::resolveColor(const std::string &str) const
{
    return ansi::utils::parseNamedOrHexColor(str);
}

ansi::RGBColor Rectangle::blend(const ansi::utils::RGBColor &src, const ansi::utils::RGBColor &dst) const
{
    float alpha = std::clamp(float(m_opacity) / 255.0f, 0.0f, 1.0f);
    auto blendChannel = [=](uint8_t fg, uint8_t bg) -> uint8_t {
        return static_cast<uint8_t>((fg * alpha) + (bg * (1.0f - alpha)));
    };

    return ansi::RGBColor{
        blendChannel(src.red(),   dst.red()),
        blendChannel(src.green(), dst.green()),
        blendChannel(src.blue(),  dst.blue())
    };
}


ansi::Attributes::Ptr Rectangle::updateAttributesForRow(int step, int totalSteps) const
{
    ansi::Attributes attr;
    ansi::RGBColor base = resolveColor(m_color).value_or(ansi::RGBColor{0, 0, 0});
    if (m_gradient) {
        // Guard against divide by zero
        int safeTotal = std::max(1, totalSteps - 1);
        int percent = (step * 100) / safeTotal;
        base = m_gradient->colorAt(percent);
    }

    if (m_opacity < 255) {
        ansi::RGBColor parentBg = [this]() -> ansi::RGBColor {
            // Optimization note: dynamic_cast is acceptable here if hierarchy is shallow
            auto *item = dynamic_cast<JobTuiItem *>(parent());
            if (!item)
                return {0, 0, 0};

            if (auto attrs = item->attributes())
                if (auto *bg = attrs->getBackground())
                    return *bg;

            return {0, 0, 0};
        }();

        base = blend(base, parentBg);
    }

    attr.setBackground(base);
    attr.setForeground(resolveColor(m_borderColor).value_or(ansi::RGBColor{255, 255, 255}));

    return ansi::Attributes::intern(attr);
}
}
