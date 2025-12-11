#pragma once
#include <functional>
#include <string>

#include "job_tui_item.h"
#include "job_tui_draw_context.h"

namespace job::tui::gui {

class Button : public JobTuiItem {
public:
    using Ptr = std::shared_ptr<Button>;    
    static Button::Ptr create(Rect rect, const std::string &text,
                              JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Button>(parent);
        ptr->setGeometry(rect);
        ptr->setText(text);
        if (parent) parent->addChild(ptr);
        return ptr;
    }

    explicit Button(JobTuiItem *parent = nullptr);
    // State
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Content
    void setText(const std::string &text);
    std::string text() const;

    // Events
    std::function<void()> onClicked;

    // Styling(optional)
    void setNormalAttributes(ansi::Attributes::Ptr a);
    void setFocusedAttributes(ansi::Attributes::Ptr a);
    void setPressedAttributes(ansi::Attributes::Ptr a);
    void setDisabledAttributes(ansi::Attributes::Ptr a);

    // Base hooks
    void paintSelf(DrawContext &ctx) override;
    bool focusEvent(const Event &event) override;
    bool mouseEvent(const Event &event) override;

private:
    enum class Visual {
        Normal,
        Focused,
        Pressed,
        Disabled
    };
    Visual visualState() const;
    void applyVisualAttributes(DrawContext &ctx) const;
    void drawLabel(DrawContext &ctx, int x, int y, int w, int h) const;

    std::string m_text          = "Button";
    bool m_enabled              = true;
    bool m_pressed              = false;
    ansi::Attributes::Ptr       m_attrNormal;
    ansi::Attributes::Ptr       m_attrFocused;
    ansi::Attributes::Ptr       m_attrPressed;
    ansi::Attributes::Ptr       m_attrDisabled;
};

}
