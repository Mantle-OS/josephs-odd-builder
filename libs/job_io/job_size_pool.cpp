#include "job_size_pool.h"

#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>

namespace job::io {

JobSizePool::JobSizePool(JobMemSize sizeClass, JobPagePool::Ptr pagePool) :
    m_sizeClass(std::move(sizeClass))
{
    init(std::move(pagePool));
}

JobSizePool::~JobSizePool()
{
    clear();
}

JobSizePool::JobSizePool(JobSizePool &&other) noexcept :
    m_sizeClass(other.m_sizeClass)
{
    std::lock_guard lock(other.m_mutex);
    moveFrom(std::move(other));
}

JobSizePool &JobSizePool::operator=(JobSizePool &&other) noexcept
{
    if (this == &other)
        return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    if (m_pagePool) {
        for (const auto &allocation : m_pageAllocations) {
            if (allocation.addr != nullptr)
                (void)m_pagePool->free(allocation.addr);
        }
    }

    m_pagePool.reset();
    m_pageAllocations.clear();
    m_freeObjects.clear();
    m_allocations.clear();
    m_objectCapacity = 0;
    m_allocatedObjects = 0;

    moveFrom(std::move(other));

    return *this;
}

JobSizePool::Ptr JobSizePool::createShared(JobMemSize sizeClass, JobPagePool::Ptr pagePool)
{
    return std::make_shared<JobSizePool>(std::move(sizeClass), std::move(pagePool));
}

JobSizePool::UPtr JobSizePool::createUniq(JobMemSize sizeClass, JobPagePool::Ptr pagePool)
{
    return std::make_unique<JobSizePool>(std::move(sizeClass), std::move(pagePool));
}

JobMemPool::Type JobSizePool::type() const noexcept
{
    return Type::Size;
}

void *JobSizePool::alloc(std::size_t size, std::size_t alignment)
{
    if (!validRequest(size, alignment))
        return nullptr;

    std::lock_guard lock(m_mutex);

    if (!m_pagePool)
        return nullptr;

    if (m_freeObjects.empty() && !grow())
        return nullptr;

    if (m_freeObjects.empty())
        return nullptr;

    void *ptr = m_freeObjects.back();
    m_freeObjects.pop_back();

    try {
        const auto [allocation, inserted] =
            m_allocations.emplace(ptr, Allocation{size});

        if (!inserted) {
            m_freeObjects.push_back(ptr);
            return nullptr;
        }
    } catch (...) {
        m_freeObjects.push_back(ptr);
        throw;
    }

    ++m_allocatedObjects;

    return ptr;
}

void *JobSizePool::allocObject()
{
    return alloc(m_sizeClass.objectSize(), m_sizeClass.alignment());
}

bool JobSizePool::free(void *ptr)
{
    if (ptr == nullptr)
        return false;

    std::lock_guard lock(m_mutex);

    if (!ownsLocked(ptr))
        return false;

    if (!isObjectBoundaryLocked(ptr))
        return false;

    const auto allocation = m_allocations.find(ptr);

    if (allocation == m_allocations.end())
        return false;

    m_freeObjects.push_back(ptr);
    m_allocations.erase(allocation);

    --m_allocatedObjects;

    return true;
}

bool JobSizePool::owns(const void *ptr) const noexcept
{
    std::lock_guard lock(m_mutex);
    return ownsLocked(ptr);
}

std::size_t JobSizePool::size() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_objectCapacity * m_sizeClass.objectSize();
}

std::size_t JobSizePool::allocated() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_allocatedObjects * m_sizeClass.objectSize();
}

std::size_t JobSizePool::available() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_freeObjects.size() * m_sizeClass.objectSize();
}

JobMemPool::Metrics JobSizePool::metrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return metricsLocked();
}

JobSizePool::SizeMetrics JobSizePool::sizeMetrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return sizeMetricsLocked();
}

void JobSizePool::clear()
{
    std::lock_guard lock(m_mutex);

    if (m_pagePool) {
        for (const auto &allocation : m_pageAllocations) {
            if (allocation.addr != nullptr)
                (void)m_pagePool->free(allocation.addr);
        }
    }

    m_pageAllocations.clear();
    m_freeObjects.clear();
    m_allocations.clear();

    m_objectCapacity = 0;
    m_allocatedObjects = 0;
}

const JobMemSize &JobSizePool::sizeClass() const noexcept
{
    return m_sizeClass;
}

std::size_t JobSizePool::objectSize() const noexcept
{
    return m_sizeClass.objectSize();
}

std::size_t JobSizePool::alignment() const noexcept
{
    return m_sizeClass.alignment();
}

std::size_t JobSizePool::objectCapacity() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_objectCapacity;
}

std::size_t JobSizePool::allocatedObjects() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_allocatedObjects;
}

std::size_t JobSizePool::availableObjects() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_freeObjects.size();
}

JobPagePool::Ptr JobSizePool::pagePool() noexcept
{
    std::lock_guard lock(m_mutex);
    return m_pagePool;
}

std::shared_ptr<const JobPagePool> JobSizePool::pagePool() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_pagePool;
}

bool JobSizePool::grow()
{
    if (!m_pagePool)
        return false;

    const std::size_t pageCount = m_sizeClass.pagesPerSpan();

    if (pageCount == 0)
        return false;

    const std::size_t objectCount = m_sizeClass.objectsPerSpan();

    if (objectCount == 0)
        return false;

    void *addr = m_pagePool->allocPages(pageCount);

    if (addr == nullptr)
        return false;

    try {
        m_pageAllocations.emplace_back(
            PageAllocation{
                .addr = addr,
                .pageCount = pageCount,
                .objectCount = objectCount
            });

        buildFreeList(addr, objectCount);
    } catch (...) {
        if (!m_pageAllocations.empty() &&
            m_pageAllocations.back().addr == addr)
            m_pageAllocations.pop_back();

        (void)m_pagePool->free(addr);
        throw;
    }

    m_objectCapacity += objectCount;

    return true;
}

void JobSizePool::buildFreeList(void *addr, std::size_t objectCount)
{
    if (addr == nullptr || objectCount == 0)
        return;

    const std::size_t objectSize = m_sizeClass.objectSize();

    auto *base = static_cast<std::byte *>(addr);

    const std::size_t oldSize = m_freeObjects.size();

    m_freeObjects.reserve(oldSize + objectCount);

    for (std::size_t i = 0; i < objectCount; ++i)
        m_freeObjects.push_back(base + i * objectSize);
}

bool JobSizePool::validRequest(std::size_t size, std::size_t alignment) const noexcept
{
    if (size == 0)
        return false;

    if (!JobMemSize::validAlignment(alignment))
        return false;

    if (size > m_sizeClass.objectSize())
        return false;

    if (alignment > m_sizeClass.alignment())
        return false;

    if (m_sizeClass.alignment() % alignment != 0)
        return false;

    return true;
}

bool JobSizePool::ownsLocked(const void *ptr) const noexcept
{
    if (ptr == nullptr)
        return false;

    const auto address = reinterpret_cast<std::uintptr_t>(ptr);

    const std::size_t spanSize = m_sizeClass.spanSize();

    for (const auto &allocation : m_pageAllocations) {
        if (allocation.addr == nullptr)
            continue;

        const auto first =
            reinterpret_cast<std::uintptr_t>(allocation.addr);

        if (spanSize >
            std::numeric_limits<std::uintptr_t>::max() - first)
            continue;

        const auto last = first + spanSize;

        if (address >= first && address < last)
            return true;
    }

    return false;
}

bool JobSizePool::isObjectBoundaryLocked(const void *ptr) const noexcept
{
    if (ptr == nullptr)
        return false;

    const auto address = reinterpret_cast<std::uintptr_t>(ptr);

    const std::size_t objectSize = m_sizeClass.objectSize();

    for (const auto &allocation : m_pageAllocations) {
        if (allocation.addr == nullptr)
            continue;

        const auto first =
            reinterpret_cast<std::uintptr_t>(allocation.addr);

        if (address < first)
            continue;

        const std::uintptr_t distance = address - first;

        const std::size_t objectBytes =
            allocation.objectCount * objectSize;

        if (distance >= objectBytes)
            continue;

        return distance % objectSize == 0;
    }

    return false;
}

JobMemPool::Metrics JobSizePool::metricsLocked() const noexcept
{
    Metrics result{};

    result.capacityBytes =
        m_objectCapacity * m_sizeClass.objectSize();

    result.allocatedBytes =
        m_allocatedObjects * m_sizeClass.objectSize();

    result.freeBytes =
        m_freeObjects.size() * m_sizeClass.objectSize();

    result.allocationCount =
        m_allocations.size();

    return result;
}

JobSizePool::SizeMetrics JobSizePool::sizeMetricsLocked() const noexcept
{
    SizeMetrics result{};

    result.objectSize =
        m_sizeClass.objectSize();

    result.alignment =
        m_sizeClass.alignment();

    result.objectCapacity =
        m_objectCapacity;

    result.allocatedObjects =
        m_allocatedObjects;

    result.availableObjects =
        m_freeObjects.size();

    result.spanCount =
        m_pageAllocations.size();

    result.pagesPerSpan =
        m_sizeClass.pagesPerSpan();

    result.reservedBytes =
        m_pageAllocations.size() * m_sizeClass.spanSize();

    result.internalWasteBytes =
        m_pageAllocations.size() * m_sizeClass.wastePerSpan();

    return result;
}

void JobSizePool::init(JobPagePool::Ptr pagePool)
{
    m_pagePool.reset();
    m_pageAllocations.clear();
    m_freeObjects.clear();
    m_allocations.clear();
    m_objectCapacity = 0;
    m_allocatedObjects = 0;

    if (!pagePool)
        return;

    if (pagePool->pageSize() != m_sizeClass.pageSize())
        return;

    if (m_sizeClass.pageSize() % m_sizeClass.alignment() != 0)
        return;

    if (m_sizeClass.objectsPerSpan() == 0)
        return;

    m_pagePool = std::move(pagePool);
}

void JobSizePool::moveFrom(JobSizePool &&other) noexcept
{
    m_sizeClass = std::move(other.m_sizeClass);

    m_pagePool = std::move(other.m_pagePool);
    m_pageAllocations = std::move(other.m_pageAllocations);
    m_freeObjects = std::move(other.m_freeObjects);
    m_allocations = std::move(other.m_allocations);

    m_objectCapacity = other.m_objectCapacity;
    m_allocatedObjects = other.m_allocatedObjects;

    other.m_pagePool.reset();
    other.m_pageAllocations.clear();
    other.m_freeObjects.clear();
    other.m_allocations.clear();
    other.m_objectCapacity = 0;
    other.m_allocatedObjects = 0;
}

} // namespace job::io