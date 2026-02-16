#include "job_ansi_scrollback.h"

namespace job::ansi {

ScrollbackBuffer::ScrollbackBuffer(size_t maxLines) :
    m_maxLines(maxLines)
{
}

const Cell::Line &ScrollbackBuffer::operator[](size_t index) const noexcept
{
    return m_lines[index];
}

Cell::Line &ScrollbackBuffer::operator[](size_t index) noexcept
{
    return m_lines[index];
}

ScrollbackBuffer::const_reverse_iterator ScrollbackBuffer::crend() const noexcept
{
    return m_lines.crend();
}

ScrollbackBuffer::const_reverse_iterator ScrollbackBuffer::crbegin() const noexcept
{
    return m_lines.crbegin();
}

ScrollbackBuffer::const_reverse_iterator ScrollbackBuffer::rend() const noexcept
{
    return m_lines.rend();
}

ScrollbackBuffer::const_reverse_iterator ScrollbackBuffer::rbegin() const noexcept
{
    return m_lines.rbegin();
}

ScrollbackBuffer::reverse_iterator ScrollbackBuffer::rend() noexcept
{
    return m_lines.rend();
}

ScrollbackBuffer::reverse_iterator ScrollbackBuffer::rbegin() noexcept
{
    return m_lines.rbegin();
}

ScrollbackBuffer::const_iterator ScrollbackBuffer::cend() const noexcept
{
    return m_lines.cend();
}

ScrollbackBuffer::const_iterator ScrollbackBuffer::cbegin() const noexcept
{
    return m_lines.cbegin();
}

ScrollbackBuffer::const_iterator ScrollbackBuffer::end() const noexcept
{
    return m_lines.end();
}

ScrollbackBuffer::const_iterator ScrollbackBuffer::begin() const noexcept
{
    return m_lines.begin();
}

ScrollbackBuffer::iterator ScrollbackBuffer::end() noexcept
{
    return m_lines.end();
}

ScrollbackBuffer::iterator ScrollbackBuffer::begin() noexcept
{
    return m_lines.begin();
}

size_t ScrollbackBuffer::size() const noexcept
{
    return m_lines.size();
}

size_t ScrollbackBuffer::maxLines() const noexcept
{
    return m_maxLines;
}

bool ScrollbackBuffer::empty() const noexcept
{
    return m_lines.empty();
}

void ScrollbackBuffer::setMaxLines(size_t maxLines)
{
    m_maxLines = maxLines;

    // Prune if we are now over capacity
    while (m_lines.size() > m_maxLines) {
        m_lines.pop_front();
    }
}

const Cell::Line &ScrollbackBuffer::at(size_t index) const
{
    if (index >= m_lines.size())
        throw std::out_of_range("ScrollbackBuffer::at index out of range");

    return m_lines[index];
}

Cell::Line &ScrollbackBuffer::at(size_t index)
{
    if (index >= m_lines.size())
        throw std::out_of_range("ScrollbackBuffer::at index out of range");

    return m_lines[index];
}

const Cell::Line &ScrollbackBuffer::back() const
{
    return m_lines.back();
}

Cell::Line &ScrollbackBuffer::back()
{
    if (m_lines.empty()) {
        throw std::out_of_range("ScrollbackBuffer::back() called on empty buffer");
    }
    return m_lines.back();
}

const Cell::Line &ScrollbackBuffer::front() const
{
    return m_lines.front();
}

Cell::Line &ScrollbackBuffer::front()
{
    return m_lines.front();
}

bool ScrollbackBuffer::push_back(Cell::Line &&line)
{
    if (m_maxLines == 0)
        return false;

    m_lines.push_back(std::move(line));

    if (m_lines.size() > m_maxLines) {
        m_lines.pop_front();
        return true; // Dropped a line
    }

    return false;
}

void ScrollbackBuffer::pop_front()
{
    if (!m_lines.empty())
        m_lines.pop_front();
}

void ScrollbackBuffer::clear() noexcept
{
    m_lines.clear();
}


} // namespace job::ansi
