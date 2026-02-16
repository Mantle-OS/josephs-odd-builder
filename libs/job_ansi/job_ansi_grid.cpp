#include "job_ansi_grid.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace job::ansi {

Grid::Grid(int rows, int cols)
{
    resize(std::max(1, rows), std::max(1, cols));
}

Grid::ConstRowView Grid::operator[](int row) const noexcept
{
    return { m_cells.data() + (row * m_columns), static_cast<size_t>(m_columns) };
}

const Cell &Grid::operator[](int row, int col) const noexcept
{
    return m_cells[row * m_columns + col];
}

Cell &Grid::operator[](int row, int col) noexcept
{
    // Warning: No bounds check in raw [] operator for speed
    return m_cells[row * m_columns + col];
}

Grid::RowView Grid::operator[](int row) noexcept
{
    return { m_cells.data() + (row * m_columns), static_cast<size_t>(m_columns) };
}

Grid::iterator Grid::begin() noexcept
{
    return m_cells.begin();
}

Grid::iterator Grid::end() noexcept
{
    return m_cells.end();
}

Grid::const_iterator Grid::begin() const noexcept
{
    return m_cells.begin();
}

Grid::const_iterator Grid::end() const noexcept
{
    return m_cells.end();
}

Grid::const_iterator Grid::cbegin() const noexcept
{
    return m_cells.cbegin();
}

Grid::const_iterator Grid::cend() const noexcept
{
    return m_cells.cend();
}

size_t Grid::size() const noexcept
{
    return m_cells.size();
}

bool Grid::empty() const noexcept
{
    return m_cells.empty();
}

int Grid::rows() const noexcept
{
    return m_rows;
}

int Grid::columns() const noexcept
{
    return m_columns;
}

Cell *Grid::at(int row, int col) noexcept
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_columns)
        return nullptr;

    return &m_cells[row * m_columns + col];
}

const Cell *Grid::at(int row, int col) const noexcept
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_columns)
        return nullptr;

    return &m_cells[row * m_columns + col];
}


void Grid::resize(int newRows, int newCols)
{
    if (newRows <= 0 || newCols <= 0)
        return;

    std::vector<Cell> newCells;
    newCells.resize(static_cast<size_t>(newRows * newCols), Cell{});

    if (!m_cells.empty()) {
        int copyRows = std::min(m_rows, newRows);
        int copyCols = std::min(m_columns, newCols);

        for (int r = 0; r < copyRows; ++r) {
            auto srcStart = m_cells.begin() + (r * m_columns);
            auto dstStart = newCells.begin() + (r * newCols);
            std::move(srcStart, srcStart + copyCols, dstStart);
        }
    }
    m_cells = std::move(newCells);
    m_rows = newRows;
    m_columns = newCols;
}

void Grid::reset(const Cell &defaultCell)
{
    std::fill(m_cells.begin(), m_cells.end(), defaultCell);
}

void Grid::push_back(const Cell &fill)
{
    if (m_columns <= 0)
        return;

    m_cells.insert(m_cells.end(), m_columns, fill);
    m_rows++;
}

void Grid::pop_front()
{
    if (m_rows <= 0)
        return;

    auto firstRowEnd = m_cells.begin() + m_columns;
    m_cells.erase(m_cells.begin(), firstRowEnd);
    m_rows--;
}

Grid::RowView Grid::row(int rowIdx)
{
    if (rowIdx < 0 || rowIdx >= m_rows)
        throw std::out_of_range("Grid::row index out of bounds");

    return {
            m_cells.data() + (rowIdx * m_columns),
            static_cast<size_t>(m_columns)
    };
}

Grid::ConstRowView Grid::row(int rowIdx) const
{
    if (rowIdx < 0 || rowIdx >= m_rows)
        throw std::out_of_range("Grid::row index out of bounds");

    return { m_cells.data() + (rowIdx * m_columns), static_cast<size_t>(m_columns) };
}

void Grid::insertLines(int startRow, int count, const Cell& fill, int limitRow)
{
    int effectiveLimit = (limitRow < 0) ? m_rows : std::min(limitRow, m_rows);

    if (startRow < 0 || startRow >= effectiveLimit || count <= 0)
        return;

    int rowsToMove = effectiveLimit - startRow - count;

    if (rowsToMove > 0) {
        // Shift lines DOWN, stopping at effectiveLimit
        for (int r = effectiveLimit - 1; r >= startRow + count; --r) {
            int srcRow = r - count;
            auto src = m_cells.begin() + (srcRow * m_columns);
            auto dst = m_cells.begin() + (r * m_columns);
            std::move(src, src + m_columns, dst);
        }
    }

    int clearEnd = std::min(effectiveLimit, startRow + count);
    for (int r = startRow; r < clearEnd; ++r) {
        auto rowStart = m_cells.begin() + (r * m_columns);
        std::fill(rowStart, rowStart + m_columns, fill);
    }
}

void Grid::deleteLines(int startRow, int count, const Cell &fill, int limitRow)
{
    int effectiveLimit = (limitRow < 0) ? m_rows : std::min(limitRow, m_rows);

    if (startRow < 0 || startRow >= effectiveLimit || count <= 0)
        return;

    // Shift lines UP, pulling from below up to effectiveLimit
    int rowsToMove = effectiveLimit - (startRow + count);

    if (rowsToMove > 0) {
        for (int r = startRow; r < startRow + rowsToMove; ++r) {
            int srcRow = r + count;
            auto src = m_cells.begin() + (srcRow * m_columns);
            auto dst = m_cells.begin() + (r * m_columns);
            std::move(src, src + m_columns, dst);
        }
    }

    // Clear the lines at the bottom of the region
    int clearStart = std::max(startRow, effectiveLimit - count);
    for (int r = clearStart; r < effectiveLimit; ++r) {
        auto rowStart = m_cells.begin() + (r * m_columns);
        std::fill(rowStart, rowStart + m_columns, fill);
    }
}

} // namespace job::ansi
