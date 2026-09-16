#pragma once

#include <deque>
#include <vector>

#include "job_ansi_cell.h"

namespace job::ansi {

class ScrollbackBuffer
{
public:
    using iterator                  = std::deque<Cell::Line>::iterator;
    using const_iterator            = std::deque<Cell::Line>::const_iterator;
    using reverse_iterator          = std::deque<Cell::Line>::reverse_iterator;
    using const_reverse_iterator    = std::deque<Cell::Line>::const_reverse_iterator;

    ScrollbackBuffer() = default;
    explicit ScrollbackBuffer(size_t maxLines);
    ScrollbackBuffer(const ScrollbackBuffer&) = default;
    ScrollbackBuffer(ScrollbackBuffer&&) noexcept = default;
    ScrollbackBuffer& operator=(const ScrollbackBuffer&) = default;
    ScrollbackBuffer& operator=(ScrollbackBuffer&&) noexcept = default;
    ~ScrollbackBuffer() = default;
    [[nodiscard]] Cell::Line &operator[](size_t index) noexcept;
    [[nodiscard]] const Cell::Line &operator[](size_t index) const noexcept;

    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] iterator end() noexcept;

    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] const_iterator cbegin() const noexcept;
    [[nodiscard]] const_iterator cend() const noexcept;

    [[nodiscard]] reverse_iterator rbegin() noexcept;
    [[nodiscard]] reverse_iterator rend() noexcept;
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept;
    [[nodiscard]] const_reverse_iterator rend() const noexcept;
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept;
    [[nodiscard]] const_reverse_iterator crend() const noexcept;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t maxLines() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    void setMaxLines(size_t maxLines);

    [[nodiscard]] const Cell::Line &at(size_t index) const;
    [[nodiscard]] Cell::Line &at(size_t index);

    [[nodiscard]] Cell::Line &front();
    [[nodiscard]] const Cell::Line &front() const;

    [[nodiscard]] Cell::Line &back();
    [[nodiscard]] const Cell::Line &back() const;

    bool push_back(Cell::Line &&line);
    void pop_front();
    void clear() noexcept;

private:
    size_t                  m_maxLines;
    std::deque<Cell::Line>  m_lines;
};

} // namespace job::ansi
