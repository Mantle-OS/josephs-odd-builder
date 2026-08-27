#pragma once

#include <compare>
#include <cstddef>
#include <limits>
#include <memory>

namespace job::io {

// Size-class policy descriptor.
//
// JobMemSize does not describe a concrete memory region and intentionally does
// not satisfy JobMemRegion.
//
// It defines how fixed-size objects are laid out within allocator pages and
// spans.
//
// Intrinsic invariants:
//
//     objectSize > 0
//     alignment is a non-zero power of two
//     objectSize is compatible with alignment
//     pageSize > 0
//     pagesPerSpan > 0
//     pageSize * pagesPerSpan does not overflow
//
// objectsPerPage() and objectsPerSpan() are derived from the configured
// geometry.
//
// JobMemSize::pageSize() refers to allocator page size, not OS mmap page size.
struct JobMemSize final
{
    using Ptr  = std::shared_ptr<JobMemSize>;
    using WPtr = std::weak_ptr<JobMemSize>;
    using UPtr = std::unique_ptr<JobMemSize>;

    using Id = std::size_t;

    static constexpr Id kInvalidId = std::numeric_limits<Id>::max();

    JobMemSize() = delete;

    constexpr JobMemSize(Id id,
                         std::size_t objectSize,
                         std::size_t alignment,
                         std::size_t pageSize,
                         std::size_t pagesPerSpan) noexcept
        pre(id != kInvalidId)
        pre(objectSize > 0)
        pre(validAlignment(alignment))
        pre(objectSize % alignment == 0)
        pre(pageSize > 0)
        pre(pagesPerSpan > 0)
        pre(pagesPerSpan <= std::numeric_limits<std::size_t>::max() / pageSize)
        pre(objectSize <= pageSize * pagesPerSpan) :
        m_id(id),
        m_objectSize(objectSize),
        m_alignment(alignment),
        m_pageSize(pageSize),
        m_pagesPerSpan(pagesPerSpan)
    {
    }

    constexpr JobMemSize(const JobMemSize &) noexcept = default;
    constexpr JobMemSize(JobMemSize &&) noexcept = default;
    constexpr JobMemSize &operator=(const JobMemSize &) noexcept = default;
    constexpr JobMemSize &operator=(JobMemSize &&) noexcept = default;
    constexpr ~JobMemSize() = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          std::size_t objectSize,
                                          std::size_t alignment,
                                          std::size_t pageSize,
                                          std::size_t pagesPerSpan)
        pre(id != kInvalidId)
        pre(objectSize > 0)
        pre(validAlignment(alignment))
        pre(objectSize % alignment == 0)
        pre(pageSize > 0)
        pre(pagesPerSpan > 0)
        pre(pagesPerSpan <= std::numeric_limits<std::size_t>::max() / pageSize)
        pre(objectSize <= pageSize * pagesPerSpan)
    {
        return std::make_shared<JobMemSize>(id, objectSize, alignment, pageSize, pagesPerSpan);
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         std::size_t objectSize,
                                         std::size_t alignment,
                                         std::size_t pageSize,
                                         std::size_t pagesPerSpan)
        pre(id != kInvalidId)
        pre(objectSize > 0)
        pre(validAlignment(alignment))
        pre(objectSize % alignment == 0)
        pre(pageSize > 0)
        pre(pagesPerSpan > 0)
        pre(pagesPerSpan <= std::numeric_limits<std::size_t>::max() / pageSize)
        pre(objectSize <= pageSize * pagesPerSpan)
    {
        return std::make_unique<JobMemSize>(id, objectSize, alignment, pageSize, pagesPerSpan);
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr std::size_t objectSize() const noexcept
    {
        return m_objectSize;
    }

    [[nodiscard]] constexpr std::size_t alignment() const noexcept
    {
        return m_alignment;
    }

    [[nodiscard]] constexpr std::size_t pageSize() const noexcept
    {
        return m_pageSize;
    }

    [[nodiscard]] constexpr std::size_t pagesPerSpan() const noexcept
    {
        return m_pagesPerSpan;
    }

    [[nodiscard]] constexpr std::size_t spanSize() const noexcept
        post(result: result == m_pageSize * m_pagesPerSpan)
    {
        return m_pageSize * m_pagesPerSpan;
    }

    [[nodiscard]] constexpr std::size_t objectsPerPage() const noexcept
    {
        return m_pageSize / m_objectSize;
    }

    [[nodiscard]] constexpr std::size_t objectsPerSpan() const noexcept
    {
        return spanSize() / m_objectSize;
    }

    [[nodiscard]] constexpr std::size_t wastePerPage() const noexcept
    {
        return m_pageSize % m_objectSize;
    }

    [[nodiscard]] constexpr std::size_t wastePerSpan() const noexcept
    {
        return spanSize() % m_objectSize;
    }

    [[nodiscard]] constexpr bool fitsInPage() const noexcept
    {
        return m_objectSize <= m_pageSize;
    }

    [[nodiscard]] constexpr bool fitsInSpan() const noexcept
    {
        return m_objectSize <= spanSize();
    }

    [[nodiscard]] static constexpr bool validAlignment(std::size_t alignment) noexcept
    {
        return alignment != 0 && (alignment & (alignment - 1)) == 0;
    }

    [[nodiscard]] constexpr bool operator==(const JobMemSize &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobMemSize &) const noexcept = default;

private:
    Id m_id{kInvalidId};
    std::size_t m_objectSize{0};
    std::size_t m_alignment{0};
    std::size_t m_pageSize{0};
    std::size_t m_pagesPerSpan{0};
};

} // namespace job::io