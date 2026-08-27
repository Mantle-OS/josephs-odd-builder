#include "job_arena_pool.h"

#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>

#include "job_mem_range.h"

namespace job::io {

JobArenaPool::JobArenaPool(std::size_t size)
{
    init(size);
}

JobArenaPool::JobArenaPool(JobMmap::Ptr mmap)
{
    init(std::move(mmap));
}

JobArenaPool::JobArenaPool(JobMemExtent::Ptr extent)
{
    init(std::move(extent));
}

JobArenaPool::~JobArenaPool() = default;

JobArenaPool::JobArenaPool(JobArenaPool &&other) noexcept
{
    std::lock_guard lock(other.m_mutex);
    moveFrom(std::move(other));
}

JobArenaPool &JobArenaPool::operator=(JobArenaPool &&other) noexcept
{
    if (this == &other)
        return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    m_extent.reset();
    m_offset = 0;
    m_paddingBytes = 0;
    m_highWatermark = 0;
    m_allocationCount = 0;

    moveFrom(std::move(other));

    return *this;
}

JobArenaPool::Ptr JobArenaPool::createShared(std::size_t size)
{
    return std::make_shared<JobArenaPool>(size);
}

JobArenaPool::Ptr JobArenaPool::createShared(JobMmap::Ptr mmap)
{
    return std::make_shared<JobArenaPool>(std::move(mmap));
}

JobArenaPool::Ptr JobArenaPool::createShared(JobMemExtent::Ptr extent)
{
    return std::make_shared<JobArenaPool>(std::move(extent));
}

JobArenaPool::UPtr JobArenaPool::createUniq(std::size_t size)
{
    return std::make_unique<JobArenaPool>(size);
}

JobArenaPool::UPtr JobArenaPool::createUniq(JobMmap::Ptr mmap)
{
    return std::make_unique<JobArenaPool>(std::move(mmap));
}

JobArenaPool::UPtr JobArenaPool::createUniq(JobMemExtent::Ptr extent)
{
    return std::make_unique<JobArenaPool>(std::move(extent));
}

JobMemPool::Type JobArenaPool::type() const noexcept
{
    return Type::Arena;
}

void *JobArenaPool::alloc(std::size_t size, std::size_t alignment)
{
    if (size == 0 || !JobMemRange::validAlignment(alignment))
        return nullptr;

    std::lock_guard lock(m_mutex);

    if (!m_extent || !m_extent->mapped())
        return nullptr;

    const std::size_t capacity = m_extent->size();

    if (m_offset > capacity)
        return nullptr;

    const auto base =
        reinterpret_cast<std::uintptr_t>(m_extent->addr());

    if (m_offset >
        std::numeric_limits<std::uintptr_t>::max() - base)
        return nullptr;

    const std::uintptr_t current =
        base + m_offset;

    std::uintptr_t aligned = 0;

    if (!alignUp(current, alignment, aligned))
        return nullptr;

    if (aligned < current)
        return nullptr;

    const std::uintptr_t paddingValue =
        aligned - current;

    if (paddingValue >
        std::numeric_limits<std::size_t>::max())
        return nullptr;

    const std::size_t padding =
        static_cast<std::size_t>(paddingValue);

    const std::size_t remaining =
        capacity - m_offset;

    if (padding > remaining)
        return nullptr;

    if (size > remaining - padding)
        return nullptr;

    const std::size_t consumed =
        padding + size;

    m_offset += consumed;
    m_paddingBytes += padding;

    if (m_offset > m_highWatermark)
        m_highWatermark = m_offset;

    ++m_allocationCount;

    return reinterpret_cast<void *>(aligned);
}

bool JobArenaPool::free(void *ptr)
{
    (void)ptr;
    return false;
}

bool JobArenaPool::owns(const void *ptr) const noexcept
{
    std::lock_guard lock(m_mutex);
    return ownsLocked(ptr);
}

std::size_t JobArenaPool::size() const noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return 0;

    return m_extent->size();
}

std::size_t JobArenaPool::allocated() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_offset;
}

std::size_t JobArenaPool::available() const noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return 0;

    const std::size_t capacity =
        m_extent->size();

    if (m_offset >= capacity)
        return 0;

    return capacity - m_offset;
}

JobMemPool::Metrics JobArenaPool::metrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return metricsLocked();
}

JobArenaPool::ArenaMetrics JobArenaPool::arenaMetrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return arenaMetricsLocked();
}

void JobArenaPool::clear()
{
    std::lock_guard lock(m_mutex);

    m_offset = 0;
    m_paddingBytes = 0;
    m_allocationCount = 0;
}

std::size_t JobArenaPool::offset() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_offset;
}

std::size_t JobArenaPool::padding() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_paddingBytes;
}

std::size_t JobArenaPool::highWatermark() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_highWatermark;
}

std::size_t JobArenaPool::allocationCount() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_allocationCount;
}

JobMemExtent::Ptr JobArenaPool::extent() noexcept
{
    std::lock_guard lock(m_mutex);
    return m_extent;
}

std::shared_ptr<const JobMemExtent> JobArenaPool::extent() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_extent;
}

JobMmap::Ptr JobArenaPool::mmap() noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return nullptr;

    return m_extent->mmap();
}

std::shared_ptr<const JobMmap> JobArenaPool::mmap() const noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return nullptr;

    return m_extent->mmap();
}

bool JobArenaPool::alignUp(
    std::uintptr_t value,
    std::size_t alignment,
    std::uintptr_t &result) noexcept
{
    if (!JobMemRange::validAlignment(alignment))
        return false;

    const std::uintptr_t mask =
        static_cast<std::uintptr_t>(alignment - 1);

    if (value >
        std::numeric_limits<std::uintptr_t>::max() - mask)
        return false;

    result =
        (value + mask) & ~mask;

    return true;
}

bool JobArenaPool::ownsLocked(const void *ptr) const noexcept
{
    return ptr != nullptr &&
           m_extent &&
           m_extent->mapped() &&
           m_extent->contains(ptr);
}

JobMemPool::Metrics JobArenaPool::metricsLocked() const noexcept
{
    Metrics result{};

    if (!m_extent)
        return result;

    result.capacityBytes =
        m_extent->size();

    result.allocatedBytes =
        m_offset;

    if (m_offset < result.capacityBytes)
        result.freeBytes =
            result.capacityBytes - m_offset;

    result.allocationCount =
        m_allocationCount;

    return result;
}

JobArenaPool::ArenaMetrics JobArenaPool::arenaMetricsLocked() const noexcept
{
    ArenaMetrics result{};

    result.usedBytes =
        m_offset;

    if (m_extent && m_offset < m_extent->size())
        result.availableBytes =
            m_extent->size() - m_offset;

    result.paddingBytes =
        m_paddingBytes;

    result.highWatermarkBytes =
        m_highWatermark;

    return result;
}

void JobArenaPool::init(std::size_t size)
{
    m_extent.reset();
    m_offset = 0;
    m_paddingBytes = 0;
    m_highWatermark = 0;
    m_allocationCount = 0;

    if (size == 0)
        return;

    init(JobMmap::createShared(size));
}

void JobArenaPool::init(JobMmap::Ptr mmap)
{
    m_extent.reset();
    m_offset = 0;
    m_paddingBytes = 0;
    m_highWatermark = 0;
    m_allocationCount = 0;

    if (!mmap ||
        !mmap->isValid() ||
        mmap->addr() == nullptr ||
        mmap->mapLength() == 0)
        return;

    const auto &mappedRanges =
        mmap->mappedRanges();

    if (mappedRanges.size() != 1)
        return;

    const JobMemRange &mapped =
        mappedRanges.front();

    if (mapped.first() != 0 ||
        mapped.last() != mmap->mapLength())
        return;

    JobMemExtent::Ptr extent =
        JobMemExtent::createShared(
            std::move(mmap),
            mapped);

    init(std::move(extent));
}

void JobArenaPool::init(JobMemExtent::Ptr extent)
{
    m_extent.reset();
    m_offset = 0;
    m_paddingBytes = 0;
    m_highWatermark = 0;
    m_allocationCount = 0;

    if (!extent || !extent->mapped())
        return;

    m_extent = std::move(extent);
}

void JobArenaPool::moveFrom(JobArenaPool &&other) noexcept
{
    m_extent                = std::move(other.m_extent);
    m_offset                = other.m_offset;
    m_paddingBytes          = other.m_paddingBytes;
    m_highWatermark         = other.m_highWatermark;
    m_allocationCount       = other.m_allocationCount;
    other.m_extent.reset();
    other.m_offset          = 0;
    other.m_paddingBytes    = 0;
    other.m_highWatermark   = 0;
    other.m_allocationCount = 0;
}

} // namespace job::io