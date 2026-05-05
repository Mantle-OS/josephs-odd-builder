#pragma once
#include <span>
#include <vector>

#include "job_ansi_cell.h"

namespace job::ansi {
class Grid {
public:
    using Ptr           = std::shared_ptr<Grid>;

    // Because these are spans
    using RowView       = std::span<Cell>;
    using ConstRowView  = std::span<const Cell>;

    using iterator       = std::vector<Cell>::iterator;
    using const_iterator = std::vector<Cell>::const_iterator;


    Grid() = default;
    Grid(int rows, int cols);
    Grid(const Grid&) = default;
    Grid(Grid&&) noexcept = default;
    Grid &operator=(const Grid&) = default;
    Grid &operator=(Grid&&) noexcept = default;

    ~Grid() = default;

    // [[nodiscard]] Cell &operator[](int row, int col) noexcept;
    // [[nodiscard]] const Cell &operator[](int row, int col) const noexcept;

    [[nodiscard]] constexpr Cell &operator()(int row, int col) noexcept {
        return m_cells[row * m_columns + col];
    }

    [[nodiscard]] constexpr const Cell &operator()(int row, int col) const noexcept {
        return m_cells[row * m_columns + col];
    }
    // c++ 23
    // [[nodiscard]] RowView operator[](int row) noexcept;
    // [[nodiscard]] ConstRowView operator[](int row) const noexcept;



    // This returns a span representing the whole row
    [[nodiscard]] constexpr RowView operator[](int row) noexcept {
        return RowView(m_cells.data() + (row * m_columns), m_columns);
    }
    [[nodiscard]] constexpr ConstRowView operator[](int row) const noexcept {
        return ConstRowView(m_cells.data() + (row * m_columns), m_columns);
    }



    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] iterator end() noexcept;

    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] const_iterator cbegin() const noexcept;
    [[nodiscard]] const_iterator cend() const noexcept;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] int rows() const noexcept;
    [[nodiscard]] int columns() const noexcept;

    [[nodiscard]] Cell *at(int row, int col) noexcept;
    [[nodiscard]] const Cell *at(int row, int col) const noexcept;

    [[nodiscard]] Grid::RowView row(int rowIdx);
    [[nodiscard]] Grid::ConstRowView row(int rowIdx) const;

    void resize(int newRows, int newCols);
    void reset(const Cell &defaultCell = Cell{});

    void push_back(const Cell &fill = Cell{});
    void pop_front();
    void insertLines(int startRow, int count, const Cell &fill, int limitRow = -1);
    void deleteLines(int startRow, int count, const Cell &fill, int limitRow = -1);

private:
    int                 m_rows = 24;
    int                 m_columns = 80;
    // Layout: Row 0 [0..cols], Row 1 [0..cols],
    std::vector<Cell>   m_cells;
};

} // namespace job::ansi


