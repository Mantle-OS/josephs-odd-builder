#pragma once

#include <list>
#include <unordered_map>
#include "job_tui_item.h"
#include "job_tui_layout_engine.h"

namespace job::tui::gui {

struct GridCell {
    int row         = -1;
    int column      = -1;
    int rowSpan     = 1;
    int columnSpan  = 1;
};

class GridLayout : public LayoutEngine, public JobTuiItem {
public:
    using Ptr = std::shared_ptr<GridLayout>;
    enum class FlowDirection {
        LeftToRight,
        TopToBottom
    };

    static GridLayout::Ptr create(JobTuiItem *parent = nullptr)
    {
        auto ptr = std::make_shared<GridLayout>(parent);
        if (parent){
            parent->setLayout(ptr);
            parent->addChild(ptr);
        }
        return ptr;
    }

    static GridLayout::Ptr create(Rect rect, JobTuiItem *parent)
    {
        auto ptr = std::make_shared<GridLayout>(parent);
        ptr->setGeometry(rect);
        if (parent){
            parent->setLayout(ptr);
            parent->addChild(ptr);
        }

        return ptr;
    }

    static GridLayout::Ptr create(Rect rect,
                                  int columns,
                                  int spacing,
                                  std::list<JobTuiItem::Ptr> children,
                                  JobTuiItem *parent)
    {
        auto ptr = std::make_shared<GridLayout>(parent);
        ptr->setGeometry(rect);
        ptr->setColumns(columns);
        ptr->setSpacing(spacing);

        for(auto i : children){
            if(!i->parent())
                i->setParent(ptr.get());
            ptr->addItem(i.get());
            ptr->addChild(i);
        }
        if (parent){
            parent->setLayout(ptr);
            parent->addChild(ptr);
            parent->applyLayout();
            // parent->di
        }

        return ptr;
    }

    static GridLayout::Ptr create(Rect rect,
                                  int columns,
                                  int spacing,
                                  std::initializer_list<JobTuiItem::Ptr> children,
                                  JobTuiItem *parent)
    {
        return create(rect, columns, spacing, std::list<JobTuiItem::Ptr>{children}, parent);
    }

    explicit GridLayout(JobTuiItem *parent = nullptr);

    void setColumns(int columns);
    int columns() const;

    void setSpacing(int spacing);
    int spacing() const;

    void setFlowDirection(FlowDirection flow);
    FlowDirection flowDirection() const;

    void applyLayout() override;

    void setCell(JobTuiItem *item, GridCell cell);
    void addItem(JobTuiItem *item);
    void addItem(JobTuiItem *item, int row, int column, int rowSpan = 1, int colSpan = 1);

    GridCell cellFor(JobTuiItem *item) const;

    void setFixedCellSize(int width, int height);
    bool hasFixedCellSize() const;


private:
    int                                             m_spacing = 0;
    int                                             m_fixedCellWidth = 0;
    int                                             m_fixedCellHeight = 0;
    int                                             m_columns = 0;
    FlowDirection                                   m_flow = FlowDirection::LeftToRight;
    std::unordered_map<JobTuiItem*, GridCell>       m_cells;
    int                                             m_nextRow = 0;
    int                                             m_nextCol = 0;
};

} 
