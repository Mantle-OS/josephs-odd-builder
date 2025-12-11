#include "column_layout.h"
namespace job::tui::gui {

ColumnLayout::ColumnLayout(JobTuiItem *parent) :
    LinearLayout(parent)
{
    setOrientation(LayoutEngine::Orientation::Vertical);
}


}
