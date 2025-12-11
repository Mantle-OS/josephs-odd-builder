#pragma once
#include <functional>
#include <string>
#include <memory>

#include "job_tui_item.h"
#include "job_tui_draw_context.h"
namespace job::tui::gui {

class Checkbox : public JobTuiItem {
public:
    using Ptr = std::shared_ptr<Checkbox>;
    static Checkbox::Ptr create(Rect rect, const std::string &text, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<Checkbox>(parent);
        ptr->setGeometry(rect);
        ptr->setText(text);
        if (parent)
            parent->addChild(ptr);

        return ptr;
    }

    explicit Checkbox(JobTuiItem *parent = nullptr);
    // Content
    void setText(const std::string &text);
    std::string text() const;

    // State
    void setChecked(bool v);
    bool isChecked() const;

    void setEnabled(bool v);
    bool isEnabled() const;

    // Events
    std::function<void(bool)> onToggled;

    // Styling (optional)
    void setNormalAttributes(ansi::Attributes::Ptr a);
    void setFocusedAttributes(ansi::Attributes::Ptr a);
    void setDisabledAttributes(ansi::Attributes::Ptr a);

    // Base hooks
    void paintSelf(DrawContext &ctx) override;
    bool focusEvent(const Event &event) override;
    bool mouseEvent(const Event &event) override;

private:
    enum class Visual {
        Normal,
        Focused,
        Disabled
    };
    Visual visualState() const;
    void applyVisualAttributes(DrawContext &ctx) const;
    void drawCheck(DrawContext &ctx, int x, int y) const;
    void drawLabel(DrawContext &ctx, int x, int y, int w, int h) const;

    std::string             m_text = "Checkbox";
    bool                    m_checked = false;
    bool                    m_enabled = true;
    bool                    m_pressed = false;
    ansi::Attributes::Ptr   m_attrNormal;
    ansi::Attributes::Ptr   m_attrFocused;
    ansi::Attributes::Ptr   m_attrDisabled;
};

} 
