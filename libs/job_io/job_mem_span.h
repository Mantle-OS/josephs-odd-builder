#pragma once

#include <compare>
#include <cstddef>
#include <limits>
#include <memory>

#include "job_mem_extent.h"
#include "job_mem_page.h"
#include "job_mem_range.h"
#include "job_mem_region_concept.h"

namespace job::io {

// One contiguous run of allocator pages within a JobMemExtent.
//
// JobMemSpan is a descriptor only. It does not allocate memory, own pages,
// maintain free lists, or know about JobPagePool policy.
//
// Both page and byte geometry use half-open ranges:
//
//     pages: [firstPageIndex, endPageIndex)
//     bytes: [range().first(), range().last())
//
// The byte range uses the same backing-relative coordinate system as
// JobMemExtent and JobMemPage.
//
// JobMemSpan::pageSize() refers to allocator page size. It is intentionally
// independent of JobMmap::pageSize(), which describes the OS mapping page size.
//
// Intrinsic invariants:
//
//     pageCount > 0
//     pageSize > 0
//     firstPageIndex + pageCount does not overflow
//     pageCount * pageSize does not overflow
//     range.size() == pageCount * pageSize
struct JobMemSpan final
{
    using Ptr  = std::shared_ptr<JobMemSpan>;
    using WPtr = std::weak_ptr<JobMemSpan>;
    using UPtr = std::unique_ptr<JobMemSpan>;

    using Id = std::size_t;
    using Index = JobMemPage::Index;

    static constexpr Id kInvalidId = std::numeric_limits<Id>::max();
    static constexpr Index kInvalidIndex = JobMemPage::kInvalidIndex;

    JobMemSpan() = delete;

    constexpr JobMemSpan(Id id,
                         JobMemExtent::Id extentId,
                         Index firstPageIndex,
                         std::size_t pageCount,
                         std::size_t pageSize,
                         JobMemRange range) noexcept
        pre(id != kInvalidId)
        pre(extentId != JobMemExtent::kInvalidId)
        pre(firstPageIndex != kInvalidIndex)
        pre(pageCount > 0)
        pre(pageSize > 0)
        pre(pageCount <= std::numeric_limits<Index>::max() - firstPageIndex)
        pre(pageCount <= std::numeric_limits<std::size_t>::max() / pageSize)
        pre(!range.empty())
        pre(range.size() == pageCount * pageSize) :
        m_id(id),
        m_extentId(extentId),
        m_firstPageIndex(firstPageIndex),
        m_pageCount(pageCount),
        m_pageSize(pageSize),
        m_range(range)
    {
    }

    constexpr JobMemSpan(const JobMemSpan &) noexcept = default;
    constexpr JobMemSpan(JobMemSpan &&) noexcept = default;
    constexpr JobMemSpan &operator=(const JobMemSpan &) noexcept = default;
    constexpr JobMemSpan &operator=(JobMemSpan &&) noexcept = default;
    constexpr ~JobMemSpan() = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          JobMemExtent::Id extentId,
                                          Index firstPageIndex,
                                          std::size_t pageCount,
                                          std::size_t pageSize,
                                          JobMemRange range)
        pre(id != kInvalidId)
        pre(extentId != JobMemExtent::kInvalidId)
        pre(firstPageIndex != kInvalidIndex)
        pre(pageCount > 0)
        pre(pageSize > 0)
        pre(pageCount <= std::numeric_limits<Index>::max() - firstPageIndex)
        pre(pageCount <= std::numeric_limits<std::size_t>::max() / pageSize)
        pre(!range.empty())
        pre(range.size() == pageCount * pageSize)
    {
        return std::make_shared<JobMemSpan>(id, extentId, firstPageIndex, pageCount, pageSize, range);
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         JobMemExtent::Id extentId,
                                         Index firstPageIndex,
                                         std::size_t pageCount,
                                         std::size_t pageSize,
                                         JobMemRange range)
        pre(id != kInvalidId)
        pre(extentId != JobMemExtent::kInvalidId)
        pre(firstPageIndex != kInvalidIndex)
        pre(pageCount > 0)
        pre(pageSize > 0)
        pre(pageCount <= std::numeric_limits<Index>::max() - firstPageIndex)
        pre(pageCount <= std::numeric_limits<std::size_t>::max() / pageSize)
        pre(!range.empty())
        pre(range.size() == pageCount * pageSize)
    {
        return std::make_unique<JobMemSpan>(id, extentId, firstPageIndex, pageCount, pageSize, range);
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr JobMemExtent::Id extentId() const noexcept
    {
        return m_extentId;
    }

    [[nodiscard]] constexpr Index firstPageIndex() const noexcept
    {
        return m_firstPageIndex;
    }

    [[nodiscard]] constexpr Index endPageIndex() const noexcept
        post(result: result == m_firstPageIndex + m_pageCount)
    {
        return m_firstPageIndex + m_pageCount;
    }

    [[nodiscard]] constexpr std::size_t pageCount() const noexcept
    {
        return m_pageCount;
    }

    [[nodiscard]] constexpr std::size_t pageSize() const noexcept
    {
        return m_pageSize;
    }

    [[nodiscard]] constexpr const JobMemRange &range() const noexcept
    {
        return m_range;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return m_range.size();
    }

    [[nodiscard]] constexpr std::size_t first() const noexcept
    {
        return m_range.first();
    }

    [[nodiscard]] constexpr std::size_t last() const noexcept
    {
        return m_range.last();
    }

    [[nodiscard]] constexpr bool contains(std::size_t offset) const noexcept
    {
        return m_range.contains(offset);
    }

    [[nodiscard]] constexpr bool containsPageIndex(Index index) const noexcept
    {
        return index >= m_firstPageIndex && index < endPageIndex();
    }

    [[nodiscard]] constexpr bool contains(const JobMemPage &page) const noexcept
    {
        return m_extentId == page.extentId() &&
               m_pageSize == page.pageSize() &&
               containsPageIndex(page.index()) &&
               m_range.contains(page.range());
    }

    [[nodiscard]] constexpr bool contains(const JobMemSpan &other) const noexcept
    {
        return m_extentId == other.m_extentId &&
               m_pageSize == other.m_pageSize &&
               other.m_firstPageIndex >= m_firstPageIndex &&
               other.endPageIndex() <= endPageIndex() &&
               m_range.contains(other.m_range);
    }

    [[nodiscard]] constexpr bool sameExtent(const JobMemSpan &other) const noexcept
    {
        return m_extentId == other.m_extentId;
    }

    [[nodiscard]] constexpr bool adjacent(const JobMemSpan &other) const noexcept
    {
        if (!sameExtent(other) || m_pageSize != other.m_pageSize)
            return false;

        const bool pageAdjacent =
            endPageIndex() == other.m_firstPageIndex ||
            other.endPageIndex() == m_firstPageIndex;

        return pageAdjacent && m_range.adjacent(other.m_range);
    }

    [[nodiscard]] constexpr bool operator==(const JobMemSpan &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobMemSpan &) const noexcept = default;

private:
    Id m_id{kInvalidId};
    JobMemExtent::Id m_extentId{JobMemExtent::kInvalidId};
    Index m_firstPageIndex{kInvalidIndex};
    std::size_t m_pageCount{0};
    std::size_t m_pageSize{0};
    JobMemRange m_range;
};

static_assert(JobMemRegion<JobMemSpan>);

} // namespace job::io