#pragma once
#include "job_tui_item.h"
namespace job::tui::gui {
class LinearLayout : public LayoutEngine, public JobTuiItem {
public:
    using Ptr = std::shared_ptr<LinearLayout>;

    static LinearLayout::Ptr create(JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<LinearLayout>(parent);
        if (parent){
            parent->setLayout(ptr);
            parent->addChild(ptr);
        }
        return ptr;
    }

    static LinearLayout::Ptr create(JobTuiItem *parent, LayoutEngine::Orientation orient)
    {
        auto ptr = std::make_shared<LinearLayout>(parent);
        ptr->setOrientation(orient);
        if (parent){
            parent->setLayout(ptr);
            parent->addChild(ptr);
        }
        return ptr;
    }

    static LinearLayout::Ptr create(Rect rect, JobTuiItem *parent, LayoutEngine::Orientation orient = LayoutEngine::Orientation::Vertical)
    {
        auto ptr = std::make_shared<LinearLayout>(parent);
        ptr->setGeometry(rect);
        ptr->setOrientation(orient);
        if (parent) {
            parent->setLayout(ptr);
            parent->addChild(ptr);
        }

        return ptr;
    }

    explicit LinearLayout(JobTuiItem *parent = nullptr);

    void setOrientation(LayoutEngine::Orientation orient);
    LayoutEngine::Orientation orientation() const;
    void setSpacing(int spacing);
    int spacing() const;
    void applyLayout() override;

private:
    LayoutEngine::Orientation       m_orientation = Orientation::Vertical;
    int                             m_spacing = 0;
};

} 
