#pragma once

#include <string>
#include <functional>
#include <memory>

#include "job_tui_item.h"
#include "job_tui_draw_context.h"
#include "job_tui_event.h"

namespace job::tui::gui {

class RadioButton : public JobTuiItem {
public:
    using Ptr = std::shared_ptr<RadioButton>;

    static RadioButton::Ptr create(JobTuiItem *parent);
    static RadioButton::Ptr create(Rect rect, const std::string &text, JobTuiItem *parent);

    void setText(const std::string &text);
    const std::string &text() const;

    void setGroupName(const std::string &name);
    const std::string &groupName() const;

    void setChecked(bool on);
    bool isChecked() const;

    // Signals/callbacks
    std::function<void(bool)> onToggled;
    std::function<void()> onClicked;

    void paintSelf(DrawContext &ctx) override;
    bool mouseEvent(const Event &event) override;
    bool focusEvent(const Event &event) override;

    ~RadioButton() override;

private:
    explicit RadioButton(JobTuiItem *parent);
    void registerInGroup();
    void unregisterFromGroup();
    void paintGlyphAndLabel(DrawContext &ctx);

private:
    JobTuiItem      *m_parentScope = nullptr;
    std::string     m_text       = "Radio";
    std::string     m_groupName  = "default";
    bool            m_checked    = false;
};

} 
