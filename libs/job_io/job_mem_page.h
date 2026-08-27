#pragma once

#include <cstddef>
#include <limits>
#include <memory>

#include "job_mem_extent.h"
#include "job_mem_range.h"
#include "job_mem_region_concept.h"

namespace job::io {
[[nodiscard]] inline std::size_t systemPageSize() noexcept
{
    const long pageSize = ::sysconf(_SC_PAGESIZE);

    if (pageSize <= 0)
        return 0;

    return static_cast<std::size_t>(pageSize);
}

// One allocator page within a JobMemExtent.
//
// JobMemPage is a descriptor only. It does not allocate memory, own free lists,
// or know about JobPagePool policy.
//
// The page range uses the same backing-relative coordinate system as
// JobMemExtent::range().
//
// For example:
//
//     extent range:    [32768, 65536)
//     allocator page:  [40960, 45056)
//
// The page therefore remains directly comparable with extent, span, and other
// page ranges without translating coordinate systems.
//
// JobMemPage::pageSize() refers to allocator page size. It is intentionally
// independent of JobMmap::pageSize(), which describes the OS mapping page size.
struct JobMemPage final
{
    using Ptr  = std::shared_ptr<JobMemPage>;
    using WPtr = std::weak_ptr<JobMemPage>;
    using UPtr = std::unique_ptr<JobMemPage>;

    using Id = std::size_t;
    using Index = std::size_t;

    static constexpr Id kInvalidId = std::numeric_limits<Id>::max();
    static constexpr Index kInvalidIndex = std::numeric_limits<Index>::max();

    JobMemPage() = delete;

    constexpr JobMemPage(Id id,
                         JobMemExtent::Id extentId,
                         Index index,
                         std::size_t pageSize,
                         JobMemRange range) noexcept
        pre(id != kInvalidId)
        pre(extentId != JobMemExtent::kInvalidId)
        pre(index != kInvalidIndex)
        pre(pageSize > 0)
        pre(!range.empty())
        pre(range.size() == pageSize) :
        m_id(id),
        m_extentId(extentId),
        m_index(index),
        m_pageSize(pageSize),
        m_range(range)
    {
    }

    constexpr JobMemPage(const JobMemPage &) noexcept = default;
    constexpr JobMemPage(JobMemPage &&) noexcept = default;
    constexpr JobMemPage &operator=(const JobMemPage &) noexcept = default;
    constexpr JobMemPage &operator=(JobMemPage &&) noexcept = default;
    constexpr ~JobMemPage() = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          JobMemExtent::Id extentId,
                                          Index index,
                                          std::size_t pageSize,
                                          JobMemRange range)
        pre(id != kInvalidId)
        pre(extentId != JobMemExtent::kInvalidId)
        pre(index != kInvalidIndex)
        pre(pageSize > 0)
        pre(!range.empty())
        pre(range.size() == pageSize)
    {
        return std::make_shared<JobMemPage>(id, extentId, index, pageSize, range);
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         JobMemExtent::Id extentId,
                                         Index index,
                                         std::size_t pageSize,
                                         JobMemRange range)
        pre(id != kInvalidId)
        pre(extentId != JobMemExtent::kInvalidId)
        pre(index != kInvalidIndex)
        pre(pageSize > 0)
        pre(!range.empty())
        pre(range.size() == pageSize)
    {
        return std::make_unique<JobMemPage>(id, extentId, index, pageSize, range);
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr JobMemExtent::Id extentId() const noexcept
    {
        return m_extentId;
    }

    [[nodiscard]] constexpr Index index() const noexcept
    {
        return m_index;
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

    [[nodiscard]] constexpr bool sameExtent(const JobMemPage &other) const noexcept
    {
        return m_extentId == other.m_extentId;
    }

    [[nodiscard]] constexpr bool adjacent(const JobMemPage &other) const noexcept
    {
        return sameExtent(other) && m_range.adjacent(other.m_range);
    }

    [[nodiscard]] constexpr bool operator==(const JobMemPage &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobMemPage &) const noexcept = default;



private:
    Id m_id{kInvalidId};
    JobMemExtent::Id m_extentId{JobMemExtent::kInvalidId};
    Index m_index{kInvalidIndex};
    std::size_t m_pageSize{0};
    JobMemRange m_range;
};

static_assert(JobMemRegion<JobMemPage>);

} // namespace job::io