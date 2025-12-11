#include "row_layout.h"
namespace job::tui::gui {

RowLayout::RowLayout(JobTuiItem *parent):
    LinearLayout(parent)
{
    setOrientation(LayoutEngine::Orientation::Horizontal);
}

}
