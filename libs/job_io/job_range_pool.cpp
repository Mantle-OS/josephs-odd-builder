#include "job_range_pool.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>

namespace job::io {

JobRangePool::JobRangePool(std::size_t size)
{
    init(size);
}

JobRangePool::JobRangePool(JobMmap::Ptr mmap)
{
    init(std::move(mmap));
}

JobRangePool::~JobRangePool() = default;

JobRangePool::JobRangePool(JobRangePool &&other) noexcept
{
    std::lock_guard lock(other.m_mutex);
    moveFrom(std::move(other));
}

JobRangePool &JobRangePool::operator=(JobRangePool &&other) noexcept
{
    if (this == &other)
        return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    m_mmap.reset();
    m_freeRanges.clear();
    m_allocations.clear();
    m_allocatedBytes = 0;

    moveFrom(std::move(other));

    return *this;
}

JobRangePool::Ptr JobRangePool::createShared(std::size_t size)
{
    return std::make_shared<JobRangePool>(size);
}

JobRangePool::Ptr JobRangePool::createShared(JobMmap::Ptr mmap)
{
    return std::make_shared<JobRangePool>(std::move(mmap));
}

JobRangePool::UPtr JobRangePool::createUniq(std::size_t size)
{
    return std::make_unique<JobRangePool>(size);
}

JobRangePool::UPtr JobRangePool::createUniq(JobMmap::Ptr mmap)
{
    return std::make_unique<JobRangePool>(std::move(mmap));
}

JobMemPool::Type JobRangePool::type() const noexcept
{
    return Type::Range;
}

void *JobRangePool::alloc(std::size_t size, std::size_t alignment)
{
    if (size == 0 || !JobMemRange::validAlignment(alignment))
        return nullptr;

    std::lock_guard lock(m_mutex);

    if (!m_mmap || !m_mmap->isValid())
        return nullptr;

    std::size_t rangeIndex = 0;
    std::size_t offset = 0;

    if (!findFreeRange(size, alignment, rangeIndex, offset))
        return nullptr;

    const auto [allocation, inserted] = m_allocations.emplace(offset, size);

    if (!inserted)
        return nullptr;

    try {
        splitRange(rangeIndex, offset, size);
    } catch (...) {
        m_allocations.erase(allocation);
        throw;
    }

    m_allocatedBytes += size;

    return ptrAt(offset);
}

bool JobRangePool::free(void *ptr)
{
    if (ptr == nullptr)
        return false;

    std::lock_guard lock(m_mutex);

    if (!ownsLocked(ptr))
        return false;

    const std::size_t offset = offsetOf(ptr);

    if (offset == std::numeric_limits<std::size_t>::max())
        return false;

    const auto allocation = m_allocations.find(offset);

    if (allocation == m_allocations.end())
        return false;

    const std::size_t allocationSize = allocation->second;

    insertFreeRange(JobMemRange::fromSize(offset, allocationSize));

    m_allocations.erase(allocation);
    m_allocatedBytes -= allocationSize;

    return true;
}

bool JobRangePool::owns(const void *ptr) const noexcept
{
    std::lock_guard lock(m_mutex);
    return ownsLocked(ptr);
}

std::size_t JobRangePool::size() const noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_mmap)
        return 0;

    return m_mmap->mappedSize();
}

std::size_t JobRangePool::allocated() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_allocatedBytes;
}

std::size_t JobRangePool::available() const noexcept
{
    std::lock_guard lock(m_mutex);

    std::size_t availableBytes = 0;

    for (const auto &range : m_freeRanges)
        availableBytes += range.size();

    return availableBytes;
}

JobMemPool::Metrics JobRangePool::metrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return metricsLocked();
}

JobRangePool::RangeMetrics JobRangePool::rangeMetrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return rangeMetricsLocked();
}

void JobRangePool::clear()
{
    std::lock_guard lock(m_mutex);

    m_allocations.clear();
    m_allocatedBytes = 0;
    m_freeRanges.clear();

    if (!m_mmap || !m_mmap->isValid())
        return;

    const auto &mappedRanges = m_mmap->mappedRanges();

    if (mappedRanges.size() != 1)
        return;

    const JobMemRange &mapped = mappedRanges.front();

    if (mapped.first() != 0 || mapped.last() != m_mmap->mapLength())
        return;

    m_freeRanges.emplace_back(mapped);
}

JobMmap::Ptr JobRangePool::mmap() noexcept
{
    std::lock_guard lock(m_mutex);
    return m_mmap;
}

std::shared_ptr<const JobMmap> JobRangePool::mmap() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_mmap;
}

std::size_t JobRangePool::alignUp(std::size_t value, std::size_t alignment) noexcept
{
    if (!JobMemRange::validAlignment(alignment))
        return std::numeric_limits<std::size_t>::max();

    const std::size_t mask = alignment - 1;

    if (value > std::numeric_limits<std::size_t>::max() - mask)
        return std::numeric_limits<std::size_t>::max();

    return (value + mask) & ~mask;
}

bool JobRangePool::findFreeRange(
    std::size_t size,
    std::size_t alignment,
    std::size_t &rangeIndex,
    std::size_t &offset) const noexcept
{
    if (!m_mmap || !m_mmap->isValid() || m_mmap->addr() == nullptr)
        return false;

    if (size == 0 || !JobMemRange::validAlignment(alignment))
        return false;

    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(m_mmap->addr());

    for (std::size_t i = 0; i < m_freeRanges.size(); ++i) {
        const JobMemRange &range = m_freeRanges[i];

        if (range.empty())
            continue;

        if (base > std::numeric_limits<std::uintptr_t>::max() - range.first())
            continue;

        const std::uintptr_t firstAddress = base + range.first();

        if (firstAddress > std::numeric_limits<std::size_t>::max())
            continue;

        const std::size_t alignedAddress = alignUp(static_cast<std::size_t>(firstAddress), alignment);

        if (alignedAddress == std::numeric_limits<std::size_t>::max())
            continue;

        if (alignedAddress < base)
            continue;

        const std::uintptr_t alignedOffsetValue = alignedAddress - base;

        if (alignedOffsetValue > std::numeric_limits<std::size_t>::max())
            continue;

        const std::size_t alignedOffset = static_cast<std::size_t>(alignedOffsetValue);

        if (alignedOffset < range.first() || alignedOffset >= range.last())
            continue;

        if (size > range.last() - alignedOffset)
            continue;

        rangeIndex = i;
        offset = alignedOffset;

        return true;
    }

    return false;
}

void JobRangePool::splitRange(
    std::size_t rangeIndex,
    std::size_t offset,
    std::size_t size)
{
    const JobMemRange original = m_freeRanges[rangeIndex];
    const std::size_t allocationLast = offset + size;

    const bool hasPrefix = original.first() < offset;
    const bool hasSuffix = allocationLast < original.last();

    if (!hasPrefix && !hasSuffix) {
        m_freeRanges.erase(m_freeRanges.begin() + static_cast<std::ptrdiff_t>(rangeIndex));
        return;
    }

    if (hasPrefix && !hasSuffix) {
        m_freeRanges[rangeIndex] = JobMemRange(original.first(), offset);
        return;
    }

    if (!hasPrefix && hasSuffix) {
        m_freeRanges[rangeIndex] = JobMemRange(allocationLast, original.last());
        return;
    }

    // Insert first so an allocation failure leaves the original range untouched.
    m_freeRanges.insert(
        m_freeRanges.begin() + static_cast<std::ptrdiff_t>(rangeIndex + 1),
        JobMemRange(allocationLast, original.last()));

    m_freeRanges[rangeIndex] = JobMemRange(original.first(), offset);
}

void JobRangePool::insertFreeRange(JobMemRange range)
{
    if (range.empty())
        return;

    m_freeRanges.emplace_back(std::move(range));
    coalesceRanges();
}

void JobRangePool::coalesceRanges()
{
    if (m_freeRanges.size() < 2)
        return;

    std::sort(
        m_freeRanges.begin(),
        m_freeRanges.end(),
        [](const JobMemRange &lhs, const JobMemRange &rhs) {
            return lhs.first() < rhs.first();
        });

    std::size_t writeIndex = 0;

    for (std::size_t readIndex = 1; readIndex < m_freeRanges.size(); ++readIndex) {
        JobMemRange &current = m_freeRanges[writeIndex];
        const JobMemRange &next = m_freeRanges[readIndex];

        if (current.mergeable(next)) {
            current = current.merged(next);
            continue;
        }

        ++writeIndex;

        if (writeIndex != readIndex)
            m_freeRanges[writeIndex] = next;
    }

    m_freeRanges.resize(writeIndex + 1);
}

std::size_t JobRangePool::offsetOf(const void *ptr) const noexcept
{
    if (ptr == nullptr || !m_mmap || m_mmap->addr() == nullptr)
        return std::numeric_limits<std::size_t>::max();

    const auto base = reinterpret_cast<std::uintptr_t>(m_mmap->addr());
    const auto address = reinterpret_cast<std::uintptr_t>(ptr);

    if (address < base)
        return std::numeric_limits<std::size_t>::max();

    const std::uintptr_t distance = address - base;

    if (distance >= m_mmap->mapLength())
        return std::numeric_limits<std::size_t>::max();

    if (distance > std::numeric_limits<std::size_t>::max())
        return std::numeric_limits<std::size_t>::max();

    return static_cast<std::size_t>(distance);
}

void *JobRangePool::ptrAt(std::size_t offset) noexcept
{
    if (!m_mmap || m_mmap->addr() == nullptr || offset >= m_mmap->mapLength())
        return nullptr;

    return static_cast<std::byte *>(m_mmap->addr()) + offset;
}

const void *JobRangePool::ptrAt(std::size_t offset) const noexcept
{
    if (!m_mmap || m_mmap->addr() == nullptr || offset >= m_mmap->mapLength())
        return nullptr;

    return static_cast<const std::byte *>(m_mmap->addr()) + offset;
}

JobMemPool::Metrics JobRangePool::metricsLocked() const noexcept
{
    Metrics result{};

    if (!m_mmap)
        return result;

    result.capacityBytes = m_mmap->mappedSize();
    result.allocatedBytes = m_allocatedBytes;

    for (const auto &range : m_freeRanges)
        result.freeBytes += range.size();

    result.allocationCount = m_allocations.size();

    return result;
}

JobRangePool::RangeMetrics JobRangePool::rangeMetricsLocked() const noexcept
{
    RangeMetrics result{};

    result.freeRangeCount = m_freeRanges.size();

    for (const auto &range : m_freeRanges)
        result.largestFreeBlock = std::max(result.largestFreeBlock, range.size());

    return result;
}

void JobRangePool::init(std::size_t size)
{
    if (size == 0)
        return;

    init(JobMmap::createShared(size));
}

void JobRangePool::init(JobMmap::Ptr mmap)
{
    m_mmap.reset();
    m_freeRanges.clear();
    m_allocations.clear();
    m_allocatedBytes = 0;

    if (!mmap || !mmap->isValid() || mmap->addr() == nullptr)
        return;

    const auto &mappedRanges = mmap->mappedRanges();

    if (mappedRanges.size() != 1)
        return;

    const JobMemRange &mapped = mappedRanges.front();

    if (mapped.first() != 0 || mapped.last() != mmap->mapLength())
        return;

    m_mmap = std::move(mmap);
    m_freeRanges.emplace_back(mapped);
}

void JobRangePool::moveFrom(JobRangePool &&other) noexcept
{
    m_mmap = std::move(other.m_mmap);
    m_freeRanges = std::move(other.m_freeRanges);
    m_allocations = std::move(other.m_allocations);
    m_allocatedBytes = other.m_allocatedBytes;

    other.m_mmap.reset();
    other.m_freeRanges.clear();
    other.m_allocations.clear();
    other.m_allocatedBytes = 0;
}

bool JobRangePool::ownsLocked(const void *ptr) const noexcept
{
    if (ptr == nullptr || !m_mmap || !m_mmap->isValid() || m_mmap->addr() == nullptr)
        return false;

    const auto base = reinterpret_cast<std::uintptr_t>(m_mmap->addr());
    const auto address = reinterpret_cast<std::uintptr_t>(ptr);

    if (address < base)
        return false;

    const std::uintptr_t distance = address - base;

    if (distance >= m_mmap->mapLength())
        return false;

    const std::size_t offset = static_cast<std::size_t>(distance);

    for (const auto &range : m_mmap->mappedRanges()) {
        if (range.contains(offset))
            return true;
    }

    return false;
}

} // namespace job::io