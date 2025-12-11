#include "grid_layout.h"

namespace job::tui::gui {

GridLayout::GridLayout(JobTuiItem *parent) :
    JobTuiItem{parent}
{
    setItemHasContents(true);
}

void GridLayout::setColumns(int columns)
{
    m_columns = columns;
}

int GridLayout::columns() const
{
    return m_columns;
}

void GridLayout::setSpacing(int spacing)
{
    m_spacing = spacing;
}

int GridLayout::spacing() const
{
    return m_spacing;
}

void GridLayout::setFlowDirection(FlowDirection flow)
{
    m_flow = flow;
}

GridLayout::FlowDirection GridLayout::flowDirection() const
{
    return m_flow;
}

void GridLayout::setFixedCellSize(int width, int height)
{
    m_fixedCellWidth = width;
    m_fixedCellHeight = height;
}

bool GridLayout::hasFixedCellSize() const
{
    return m_fixedCellWidth > 0 && m_fixedCellHeight > 0;
}

void GridLayout::setCell(JobTuiItem *item, GridCell cell)
{
    if (!item)
        return;

    if (cell.row == -1 || cell.column == -1) {
        if (m_flow == FlowDirection::LeftToRight) {
            cell.row = m_nextRow;
            cell.column = m_nextCol++;

            if (m_columns > 0 && m_nextCol >= m_columns) {
                m_nextRow++;
                m_nextCol = 0;
            }

        } else {
            cell.column = m_nextCol;
            cell.row = m_nextRow++;
        }

        if (m_nextCol > 100) {
            m_nextRow++;
            m_nextCol = 0;
        }
    }

    m_cells[item] = cell;
}

void GridLayout::addItem(JobTuiItem *item)
{
    setCell(item, GridCell{});
}

void GridLayout::addItem(JobTuiItem *item, int row, int column, int rowSpan, int colSpan)
{
    setCell(item, GridCell{ row, column, rowSpan, colSpan });
}

GridCell GridLayout::cellFor(JobTuiItem *item) const
{
    auto it = m_cells.find(item);
    return it != m_cells.end() ? it->second : GridCell{};
}

void GridLayout::applyLayout()
{
    const int originX = this->x();
    const int originY = this->y();
    const int maxW = this->width();
    const int maxH = this->height();

    int maxRow = 0;
    int maxCol = 0;

    for (const auto &[_, cell] : m_cells) {
        maxRow = std::max(maxRow, cell.row + cell.rowSpan);
        maxCol = std::max(maxCol, cell.column + cell.columnSpan);
    }

    int cellWidth = m_fixedCellWidth > 0 ? m_fixedCellWidth
                                         : (maxCol > 0 ? (maxW - (maxCol - 1) * m_spacing) / maxCol : 0);
    int cellHeight = m_fixedCellHeight > 0 ? m_fixedCellHeight
                                           : (maxRow > 0 ? (maxH - (maxRow - 1) * m_spacing) / maxRow : 0);

    for (const auto &[itemPtr, cell] : m_cells) {
        if (!itemPtr || !itemPtr->isVisible())
            continue;

        int x = originX + cell.column * (cellWidth + m_spacing);
        int y = originY + cell.row * (cellHeight + m_spacing);
        int w = cell.columnSpan * cellWidth + (cell.columnSpan - 1) * m_spacing;
        int h = cell.rowSpan * cellHeight + (cell.rowSpan - 1) * m_spacing;

        itemPtr->setGeometry(x, y, w, h);
    }
}


}
