#pragma once
#include <functional>

#include "job_tui_item.h"
namespace job::tui {

class JobTuiMouseArea : public JobTuiItem {
public:
    using Ptr = std::shared_ptr<JobTuiMouseArea>;
    explicit JobTuiMouseArea(JobTuiItem *parent = nullptr);
    static JobTuiMouseArea::Ptr create(JobTuiItem *parent, bool fill = true);
    std::function<void(const Event &)> onClicked;
    std::function<void(const Event &)> onReleased;
    std::function<void(const Event &)> onMoved;
    void paintSelf(DrawContext &) override
    {
        // do nothing — don't call paint from JobTuiItem
    }
    bool mouseEvent(const Event &event) override;
};

}
