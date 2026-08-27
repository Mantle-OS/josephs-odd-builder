#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <contracts>

#include "job_mem_range.h"
#include "job_mmap.h"

namespace job::io {

// One contiguous region within a JobMmap.
//
// JobMemExtent gives backing identity to an otherwise geometry-only
// JobMemRange.
//
// The extent range uses JobMmap-relative coordinates:
//
//     mmap()->addr() + range().first()
//
// For example:
//
//     mmap range:      [0, 65536)
//     extent range:    [32768, 49152)
//     extent addr:     mmap()->addr() + 32768
//
// Extent-local offsets are therefore:
//
//     mmapOffset = range().first() + localOffset
//
// Ranges belonging to different extents must never be coalesced merely
// because their numeric boundaries happen to be adjacent.
//
// JobMmap is mutable. An extent is required to refer to a live mapped range
// when it is created, but a later partial unmap can make that extent stale.
// Operations that require live backing therefore contract on mapped().
struct JobMemExtent final
{
    using Ptr  = std::shared_ptr<JobMemExtent>;
    using WPtr = std::weak_ptr<JobMemExtent>;
    using UPtr = std::unique_ptr<JobMemExtent>;

    using Id = std::size_t;

    static constexpr Id kInvalidId = std::numeric_limits<Id>::max();

    JobMemExtent() = delete;

    JobMemExtent(JobMmap::Ptr mmap, JobMemRange range)
        pre(mmap != nullptr)
        pre(mmap->addr() != nullptr)
        pre(!range.empty())
        pre(range.last() <= mmap->mapLength())
        pre(isMappedRange(*mmap, range)) :
        m_id(nextId()),
        m_mmap(std::move(mmap)),
        m_range(range)
    {
    }

    JobMemExtent(const JobMemExtent &) = default;
    JobMemExtent(JobMemExtent &&) noexcept = default;
    JobMemExtent &operator=(const JobMemExtent &) = default;
    JobMemExtent &operator=(JobMemExtent &&) noexcept = default;
    ~JobMemExtent() = default;

    [[nodiscard]] static Ptr createShared(JobMmap::Ptr mmap, JobMemRange range)
        pre(mmap != nullptr)
        pre(mmap->addr() != nullptr)
        pre(!range.empty())
        pre(range.last() <= mmap->mapLength())
        pre(isMappedRange(*mmap, range))
    {
        return std::make_shared<JobMemExtent>(std::move(mmap), range);
    }

    [[nodiscard]] static UPtr createUniq(JobMmap::Ptr mmap, JobMemRange range)
        pre(mmap != nullptr)
        pre(mmap->addr() != nullptr)
        pre(!range.empty())
        pre(range.last() <= mmap->mapLength())
        pre(isMappedRange(*mmap, range))
    {
        return std::make_unique<JobMemExtent>(std::move(mmap), range);
    }

    [[nodiscard]] Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] const JobMemRange &range() const noexcept
    {
        return m_range;
    }

    [[nodiscard]] std::size_t size() const noexcept
        post(result: result == m_range.size())
    {
        return m_range.size();
    }

    [[nodiscard]] JobMmap::Ptr mmap() noexcept
    {
        return m_mmap;
    }

    [[nodiscard]] std::shared_ptr<const JobMmap> mmap() const noexcept
    {
        return m_mmap;
    }

    [[nodiscard]] bool mapped() const noexcept
    {
        return m_mmap &&
               m_mmap->addr() != nullptr &&
               m_range.last() <= m_mmap->mapLength() &&
               isMappedRange(*m_mmap, m_range);
    }

    [[nodiscard]] void *addr() noexcept
        pre(mapped())
        post(result: result != nullptr)
    {
        auto *base = static_cast<std::byte *>(m_mmap->addr());
        return base + m_range.first();
    }

    [[nodiscard]] const void *addr() const noexcept
        pre(mapped())
        post(result: result != nullptr)
    {
        const auto *base = static_cast<const std::byte *>(m_mmap->addr());
        return base + m_range.first();
    }

    [[nodiscard]] bool contains(const JobMemRange &range) const noexcept
    {
        return m_range.contains(range);
    }

    [[nodiscard]] bool contains(const void *ptr) const noexcept
    {
        if (!ptr || !mapped())
            return false;

        const auto first = reinterpret_cast<std::uintptr_t>(addr());
        const auto address = reinterpret_cast<std::uintptr_t>(ptr);

        if (size() > std::numeric_limits<std::uintptr_t>::max() - first)
            return false;

        return address >= first && address < first + size();
    }

    [[nodiscard]] std::size_t offsetOf(const void *ptr) const noexcept
        pre(contains(ptr))
        post(result: result < size())
    {
        const auto first = reinterpret_cast<std::uintptr_t>(addr());
        const auto address = reinterpret_cast<std::uintptr_t>(ptr);

        return static_cast<std::size_t>(address - first);
    }

    [[nodiscard]] std::size_t mmapOffsetOf(const void *ptr) const noexcept
        pre(contains(ptr))
        post(result: m_range.contains(result))
    {
        return m_range.first() + offsetOf(ptr);
    }

    [[nodiscard]] void *ptrAt(std::size_t offset) noexcept
        pre(mapped())
        pre(offset < size())
        post(result: result != nullptr)
        post(result: contains(result))
    {
        auto *base = static_cast<std::byte *>(addr());
        return base + offset;
    }

    [[nodiscard]] const void *ptrAt(std::size_t offset) const noexcept
        pre(mapped())
        pre(offset < size())
        post(result: result != nullptr)
        post(result: contains(result))
    {
        const auto *base = static_cast<const std::byte *>(addr());
        return base + offset;
    }

    [[nodiscard]] void *ptrAtMmapOffset(std::size_t offset) noexcept
        pre(mapped())
        pre(m_range.contains(offset))
        post(result: result != nullptr)
        post(result: contains(result))
    {
        auto *base = static_cast<std::byte *>(m_mmap->addr());
        return base + offset;
    }

    [[nodiscard]] const void *ptrAtMmapOffset(std::size_t offset) const noexcept
        pre(mapped())
        pre(m_range.contains(offset))
        post(result: result != nullptr)
        post(result: contains(result))
    {
        const auto *base = static_cast<const std::byte *>(m_mmap->addr());
        return base + offset;
    }

    [[nodiscard]] bool sameBacking(const JobMemExtent &other) const noexcept
    {
        return m_mmap.get() == other.m_mmap.get();
    }

    [[nodiscard]] bool operator==(const JobMemExtent &other) const noexcept
    {
        return m_id == other.m_id &&
               m_mmap.get() == other.m_mmap.get() &&
               m_range == other.m_range;
    }

private:
    [[nodiscard]] static Id nextId() noexcept
    {
        static std::atomic<Id> next{0};

        Id id = next.fetch_add(1, std::memory_order_relaxed);

        if (id == kInvalidId)
            std::terminate();

        return id;
    }

    [[nodiscard]] static bool isMappedRange(const JobMmap &mmap, const JobMemRange &range) noexcept
    {
        for (const auto &mapped : mmap.mappedRanges()) {
            if (mapped.contains(range))
                return true;
        }

        return false;
    }

    Id m_id{kInvalidId};
    JobMmap::Ptr m_mmap;
    JobMemRange m_range;
};

} // namespace job::io