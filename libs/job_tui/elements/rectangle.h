#pragma once

#include <memory>

#include <rgb_color.h>

#include "job_tui_gradient.h"
#include "job_tui_item.h"

namespace job::tui::gui {

class Rectangle : public JobTuiItem {
public:
    using Ptr = std::shared_ptr<Rectangle>;

    static Rectangle::Ptr create(JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Rectangle>(parent);
        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    static Rectangle::Ptr create(Rect rect, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Rectangle>(parent);
        ptr->setGeometry(rect);
        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    static Rectangle::Ptr create(Rect rect, const std::string &color, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Rectangle>(parent);
        ptr->setGeometry(rect);
        ptr->setColor(color);
        if (parent)
            parent->addChild(ptr);
        return ptr;
    }

    explicit Rectangle(JobTuiItem *parent = nullptr);

    // ansi::Attributes::Ptr computeAttributeForStep(int step, int totalSteps) const;

    void paintSelf(DrawContext &ctx) override;

    std::string color() const;
    void setColor(const std::string &color);
    void setColor(ansi::utils::RGBColor color);

    Gradient *gradient() const;
    void setGradient(std::unique_ptr<Gradient> gradient);

    uint8_t radius() const;
    void setRadius(uint8_t radius);

    uint8_t opacity() const;
    void setOpacity(uint8_t newOpacity);

    bool hasBorder() const;
    void setHasBorder(bool hasBorder);

    std::string borderColor() const;
    void setBorderColor(const std::string &borderColor);
    void setBorderColor(job::ansi::utils::RGBColor color);

    uint8_t borderWidth() const;
    void setBorderWidth(uint8_t borderWidth);

    DrawContext::BorderStyle borderStyle() const;
    void setBorderStyle(DrawContext::BorderStyle style);

private:
    std::string                     m_color;
    std::unique_ptr<Gradient>       m_gradient;
    uint8_t                         m_radius = 0;
    uint8_t                         m_opacity = 255;
    bool                            m_hasBorder = false;
    std::string                     m_borderColor;
    uint8_t                         m_borderWidth = 1;
    DrawContext::BorderStyle        m_borderStyle = DrawContext::BorderStyle::BLOCK;

    std::optional<ansi::RGBColor> resolveColor(const std::string &str) const;
    ansi::RGBColor blend(const ansi::RGBColor &src, const ansi::RGBColor &dst) const;
    ansi::Attributes::Ptr updateAttributesForRow(int step, int totalSteps) const;
};

} 
