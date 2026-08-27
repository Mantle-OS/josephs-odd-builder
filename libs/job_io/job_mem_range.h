#pragma once

#include <compare>
#include <cstddef>
#include <limits>
#include <memory>
#include <contracts>

namespace job::io {

// Half-open byte range: [first, last)
//
// JobMemRange describes byte geometry only. It has no backing-memory identity
// and does not know about JobMmap, extents, pages, spans, pools, or allocation
// state.
//
// Invariant:
//     first <= last
//
// Empty ranges are valid:
//     [N, N)
struct JobMemRange final
{
    using Ptr  = std::shared_ptr<JobMemRange>;
    using WPtr = std::weak_ptr<JobMemRange>;
    using UPtr = std::unique_ptr<JobMemRange>;

    constexpr JobMemRange() noexcept = default;

    constexpr JobMemRange(std::size_t first, std::size_t last) noexcept
        pre(first <= last) :
        m_first(first),
        m_last(last)
    {
    }

    constexpr JobMemRange(const JobMemRange &) noexcept = default;
    constexpr JobMemRange(JobMemRange &&) noexcept = default;
    constexpr JobMemRange &operator=(const JobMemRange &) noexcept = default;
    constexpr JobMemRange &operator=(JobMemRange &&) noexcept = default;
    constexpr ~JobMemRange() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobMemRange>();
    }

    [[nodiscard]] static Ptr createShared(std::size_t first, std::size_t last)
        pre(first <= last)
    {
        return std::make_shared<JobMemRange>(first, last);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobMemRange>();
    }

    [[nodiscard]] static UPtr createUniq(std::size_t first, std::size_t last)
        pre(first <= last)
    {
        return std::make_unique<JobMemRange>(first, last);
    }

    [[nodiscard]] static constexpr JobMemRange fromSize(const std::size_t first, const std::size_t size) noexcept
        pre(size <= std::numeric_limits<std::size_t>::max() - first)
    {
        return JobMemRange(first, first + size);
    }

    [[nodiscard]] constexpr std::size_t first() const noexcept
    {
        return m_first;
    }

    [[nodiscard]] constexpr std::size_t last() const noexcept
    {
        return m_last;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return m_last - m_first;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_first == m_last;
    }

    [[nodiscard]] constexpr bool contains(std::size_t offset) const noexcept
    {
        return offset >= m_first && offset < m_last;
    }

    [[nodiscard]] constexpr bool contains(const JobMemRange &other) const noexcept
    {
        return other.m_first >= m_first && other.m_last <= m_last;
    }

    [[nodiscard]] constexpr bool overlaps(const JobMemRange &other) const noexcept
    {
        return m_first < other.m_last && other.m_first < m_last;
    }

    [[nodiscard]] constexpr bool adjacent(const JobMemRange &other) const noexcept
    {
        return m_last == other.m_first || other.m_last == m_first;
    }

    [[nodiscard]] constexpr bool mergeable(const JobMemRange &other) const noexcept
    {
        return overlaps(other) || adjacent(other);
    }

    [[nodiscard]] constexpr JobMemRange merged(const JobMemRange &other) const noexcept
        pre(mergeable(other))
    {
        const std::size_t first = m_first < other.m_first ? m_first : other.m_first;
        const std::size_t last = m_last > other.m_last ? m_last : other.m_last;

        return JobMemRange(first, last);
    }

    [[nodiscard]] constexpr JobMemRange intersection(const JobMemRange &other) const noexcept
        pre(overlaps(other))
    {
        const std::size_t first = m_first > other.m_first ? m_first : other.m_first;
        const std::size_t last = m_last < other.m_last ? m_last : other.m_last;

        return JobMemRange(first, last);
    }

    [[nodiscard]] constexpr JobMemRange alignedInward(std::size_t alignment) const noexcept
        pre(validAlignment(alignment))
    {
        if (empty())
            return *this;

        const std::size_t first = alignUp(m_first, alignment);
        const std::size_t last = alignDown(m_last, alignment);

        if (first >= last) {
            const std::size_t empty = first < m_last ? first : m_last;
            return JobMemRange(empty, empty);
        }

        return JobMemRange(first, last);
    }

    [[nodiscard]] constexpr JobMemRange alignedOutward(std::size_t alignment) const noexcept
        pre(validAlignment(alignment))
        pre(m_last <= std::numeric_limits<std::size_t>::max() - (alignment - 1))
    {
        if (empty())
            return *this;

        return JobMemRange(alignDown(m_first, alignment), alignUp(m_last, alignment));
    }

    [[nodiscard]] static constexpr bool validAlignment(std::size_t alignment) noexcept
    {
        return alignment != 0 && (alignment & (alignment - 1)) == 0;
    }

    [[nodiscard]] constexpr bool operator==(const JobMemRange &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobMemRange &) const noexcept = default;

private:
    [[nodiscard]] static constexpr std::size_t alignDown(std::size_t value, std::size_t alignment) noexcept
        pre(validAlignment(alignment))
    {
        return value & ~(alignment - 1);
    }

    [[nodiscard]] static constexpr std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept
        pre(validAlignment(alignment))
        pre(value <= std::numeric_limits<std::size_t>::max() - (alignment - 1))
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    std::size_t m_first{0};
    std::size_t m_last{0};
};

} // namespace job::io