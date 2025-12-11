#pragma once

#include "linear_layout.h"
#include <list>

namespace job::tui::gui {

class RowLayout : public LinearLayout {
public:
    using Ptr = std::shared_ptr<RowLayout>;
    explicit RowLayout(JobTuiItem *parent = nullptr);

    static RowLayout::Ptr create(Rect rect, JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<RowLayout>(parent);
        ptr->setGeometry(rect);
        if (parent) {
            parent->setLayout(ptr);
            parent->addChild(ptr);
            parent->applyLayout();
        }
        return ptr;
    }
    static RowLayout::Ptr create(Rect rect, int spacing,
                      std::list<JobTuiItem::Ptr> children,
                      JobTuiItem *parent)
    {
        auto ptr = std::make_shared<RowLayout>(parent);
        ptr->setGeometry(rect);
        ptr->setSpacing(spacing);
        ptr->setOrientation(LayoutEngine::Orientation::Horizontal);
        if (parent) {
            parent->setLayout(ptr);
            parent->addChild(ptr);
        }
        for (auto &child : children) {
            if (!child->parent())
                child->setParent(ptr.get());
            ptr->addChild(child);
        }
        ptr->applyLayout();
        return ptr;
    }

    static RowLayout::Ptr create(Rect rect, int spacing,
                      std::initializer_list<JobTuiItem::Ptr> children,
                      JobTuiItem *parent)
    {
        return create(rect, spacing, std::list<JobTuiItem::Ptr>{children}, parent);
    }
};

}
