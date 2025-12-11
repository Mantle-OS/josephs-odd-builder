#include "linear_layout.h"

namespace job::tui::gui{

LinearLayout::LinearLayout(JobTuiItem *parent) :
    JobTuiItem{parent}
{
    setItemHasContents(true);
}

void LinearLayout::setOrientation(LayoutEngine::Orientation orient)
{
    m_orientation = orient;
}

LayoutEngine::Orientation LinearLayout::orientation() const
{
    return m_orientation;
}

void LinearLayout::setSpacing(int spacing)
{
    m_spacing = spacing;
}

int LinearLayout::spacing() const
{
    return m_spacing;
}

void LinearLayout::applyLayout()
{
    int x = this->x();
    int y = this->y();
    int availableWidth = this->width();

    for (const auto &[key, childPtr] : this->children()) {
        if (!childPtr)
            continue;

        auto *child = dynamic_cast<JobTuiItem *>(childPtr.get());
        if (!child || !child->isVisible())
            continue;

        int itemHeight = child->height(); // Use existing height

        child->setGeometry(x, y, availableWidth, itemHeight);

        if (m_orientation == Orientation::Vertical)
            y += itemHeight + m_spacing;
        else
            x += child->width() + m_spacing;
    }
}
}
