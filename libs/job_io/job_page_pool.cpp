#include "job_page_pool.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>
#include <atomic>

namespace job::io {

JobPagePool::JobPagePool(std::size_t size, std::size_t pageSize)
{
    init(size, pageSize);
}

JobPagePool::JobPagePool(JobMmap::Ptr mmap, std::size_t pageSize)
{
    init(std::move(mmap), pageSize);
}

JobPagePool::JobPagePool(JobMemExtent::Ptr extent, std::size_t pageSize)
{
    init(std::move(extent), pageSize);
}

JobPagePool::~JobPagePool() = default;

JobPagePool::JobPagePool(JobPagePool &&other) noexcept
{
    std::lock_guard lock(other.m_mutex);
    moveFrom(std::move(other));
}

JobPagePool &JobPagePool::operator=(JobPagePool &&other) noexcept
{
    if (this == &other)
        return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    m_extent.reset();
    m_pageSize = 0;
    m_pages.clear();
    m_freeSpans.clear();
    m_allocations.clear();
    m_allocatedPages = 0;
    m_allocatedBytes = 0;
    m_nextSpanId = 0;

    moveFrom(std::move(other));

    return *this;
}

JobPagePool::Ptr JobPagePool::createShared(std::size_t size, std::size_t pageSize)
{
    return std::make_shared<JobPagePool>(size, pageSize);
}

JobPagePool::Ptr JobPagePool::createShared(JobMmap::Ptr mmap, std::size_t pageSize)
{
    return std::make_shared<JobPagePool>(std::move(mmap), pageSize);
}

JobPagePool::Ptr JobPagePool::createShared(JobMemExtent::Ptr extent, std::size_t pageSize)
{
    return std::make_shared<JobPagePool>(std::move(extent), pageSize);
}

JobPagePool::UPtr JobPagePool::createUniq(std::size_t size, std::size_t pageSize)
{
    return std::make_unique<JobPagePool>(size, pageSize);
}

JobPagePool::UPtr JobPagePool::createUniq(JobMmap::Ptr mmap, std::size_t pageSize)
{
    return std::make_unique<JobPagePool>(std::move(mmap), pageSize);
}

JobPagePool::UPtr JobPagePool::createUniq(JobMemExtent::Ptr extent, std::size_t pageSize)
{
    return std::make_unique<JobPagePool>(std::move(extent), pageSize);
}

JobMemPool::Type JobPagePool::type() const noexcept
{
    return Type::Page;
}

void *JobPagePool::alloc(std::size_t size, std::size_t alignment)
{
    if (size == 0 || !JobMemRange::validAlignment(alignment))
        return nullptr;

    std::lock_guard lock(m_mutex);

    if (!m_extent || !m_extent->mapped() || m_pageSize == 0)
        return nullptr;

    const std::size_t requiredPages = pagesForSize(size, m_pageSize);

    if (requiredPages == 0)
        return nullptr;

    std::size_t spanIndex = 0;
    JobMemPage::Index firstPageIndex = JobMemPage::kInvalidIndex;

    if (!findFreeSpan(requiredPages, alignment, spanIndex, firstPageIndex))
        return nullptr;

    Allocation allocation{
        .firstPageIndex = firstPageIndex,
        .pageCount = requiredPages,
        .requestedSize = size
    };

    const auto [entry, inserted] = m_allocations.emplace(firstPageIndex, allocation);

    if (!inserted)
        return nullptr;

    try {
        splitSpan(spanIndex, firstPageIndex, requiredPages);
    } catch (...) {
        m_allocations.erase(entry);
        throw;
    }

    const std::size_t allocationBytes = requiredPages * m_pageSize;

    m_allocatedPages += requiredPages;
    m_allocatedBytes += allocationBytes;

    return ptrAtPage(firstPageIndex);
}

void *JobPagePool::allocPages(std::size_t pageCount)
{
    if (pageCount == 0)
        return nullptr;

    std::lock_guard lock(m_mutex);

    if (!m_extent || !m_extent->mapped() || m_pageSize == 0)
        return nullptr;

    if (pageCount > std::numeric_limits<std::size_t>::max() / m_pageSize)
        return nullptr;

    std::size_t spanIndex = 0;
    JobMemPage::Index firstPageIndex = JobMemPage::kInvalidIndex;

    if (!findFreeSpan(pageCount, m_pageSize, spanIndex, firstPageIndex))
        return nullptr;

    const std::size_t allocationBytes = pageCount * m_pageSize;

    Allocation allocation{
        .firstPageIndex = firstPageIndex,
        .pageCount = pageCount,
        .requestedSize = allocationBytes
    };

    const auto [entry, inserted] = m_allocations.emplace(firstPageIndex, allocation);

    if (!inserted)
        return nullptr;

    try {
        splitSpan(spanIndex, firstPageIndex, pageCount);
    } catch (...) {
        m_allocations.erase(entry);
        throw;
    }

    m_allocatedPages += pageCount;
    m_allocatedBytes += allocationBytes;

    return ptrAtPage(firstPageIndex);
}

bool JobPagePool::free(void *ptr)
{
    if (ptr == nullptr)
        return false;

    std::lock_guard lock(m_mutex);

    if (!ownsLocked(ptr) || m_pageSize == 0)
        return false;

    const std::size_t offset = offsetOf(ptr);

    if (offset == std::numeric_limits<std::size_t>::max())
        return false;

    // free() only accepts the beginning of an allocation. An interior address
    // within the first page must not collapse to the same page index.
    if (offset % m_pageSize != 0)
        return false;

    const JobMemPage::Index pageIndex = pageIndexOf(ptr);

    if (pageIndex == JobMemPage::kInvalidIndex)
        return false;

    const auto allocation = m_allocations.find(pageIndex);

    if (allocation == m_allocations.end())
        return false;

    const std::size_t allocationPages = allocation->second.pageCount;
    const std::size_t allocationBytes = allocationPages * m_pageSize;

    JobMemSpan span = makeSpan(pageIndex, allocationPages);

    insertFreeSpan(std::move(span));

    m_allocations.erase(allocation);
    m_allocatedPages -= allocationPages;
    m_allocatedBytes -= allocationBytes;

    return true;
}

bool JobPagePool::owns(const void *ptr) const noexcept
{
    std::lock_guard lock(m_mutex);
    return ownsLocked(ptr);
}

std::size_t JobPagePool::size() const noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return 0;

    return m_extent->size();
}

std::size_t JobPagePool::allocated() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_allocatedBytes;
}

std::size_t JobPagePool::available() const noexcept
{
    std::lock_guard lock(m_mutex);

    std::size_t availableBytes = 0;

    for (const auto &span : m_freeSpans)
        availableBytes += span.size();

    return availableBytes;
}

JobMemPool::Metrics JobPagePool::metrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return metricsLocked();
}

JobPagePool::PageMetrics JobPagePool::pageMetrics() const noexcept
{
    std::lock_guard lock(m_mutex);
    return pageMetricsLocked();
}

void JobPagePool::clear()
{
    std::lock_guard lock(m_mutex);

    m_allocations.clear();
    m_freeSpans.clear();
    m_allocatedPages = 0;
    m_allocatedBytes = 0;

    if (!m_extent || !m_extent->mapped() || m_pageSize == 0)
        return;

    if (!validPageGeometry(*m_extent, m_pageSize))
        return;

    if (m_pages.empty())
        return;

    m_freeSpans.emplace_back(makeSpan(0, m_pages.size()));
}

std::size_t JobPagePool::pageSize() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_pageSize;
}

std::size_t JobPagePool::pageCount() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_pages.size();
}

std::size_t JobPagePool::allocatedPages() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_allocatedPages;
}

std::size_t JobPagePool::availablePages() const noexcept
{
    std::lock_guard lock(m_mutex);

    std::size_t result = 0;

    for (const auto &span : m_freeSpans)
        result += span.pageCount();

    return result;
}

const JobPagePool::Pages &JobPagePool::pages() const noexcept
{
    // Pages are immutable during normal allocator operation. alloc(), free()
    // and clear() manipulate allocation/span bookkeeping only.
    return m_pages;
}

JobMemExtent::Ptr JobPagePool::extent() noexcept
{
    std::lock_guard lock(m_mutex);
    return m_extent;
}

std::shared_ptr<const JobMemExtent> JobPagePool::extent() const noexcept
{
    std::lock_guard lock(m_mutex);
    return m_extent;
}

JobMmap::Ptr JobPagePool::mmap() noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return nullptr;

    return m_extent->mmap();
}

std::shared_ptr<const JobMmap> JobPagePool::mmap() const noexcept
{
    std::lock_guard lock(m_mutex);

    if (!m_extent)
        return nullptr;

    return m_extent->mmap();
}


std::size_t JobPagePool::pagesForSize(std::size_t size, std::size_t pageSize) noexcept
{
    if (size == 0 || pageSize == 0)
        return 0;

    return size / pageSize + (size % pageSize != 0 ? 1 : 0);
}

bool JobPagePool::validPageGeometry(
    const JobMemExtent &extent,
    std::size_t pageSize) const noexcept
{
    if (pageSize == 0 || !JobMemRange::validAlignment(pageSize))
        return false;

    if (!extent.mapped() || extent.size() == 0)
        return false;

    if (extent.size() % pageSize != 0)
        return false;

    const auto address = reinterpret_cast<std::uintptr_t>(extent.addr());

    if (address % pageSize != 0)
        return false;

    const std::size_t count = extent.size() / pageSize;

    if (count == 0)
        return false;

    if (count - 1 > JobMemPage::kInvalidIndex - 1)
        return false;

    return true;
}

bool JobPagePool::findFreeSpan(
    std::size_t pageCount,
    std::size_t alignment,
    std::size_t &spanIndex,
    JobMemPage::Index &firstPageIndex) const noexcept
{
    if (!m_extent || !m_extent->mapped() || m_pageSize == 0)
        return false;

    if (pageCount == 0 || !JobMemRange::validAlignment(alignment))
        return false;

    const std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(m_extent->addr());

    const std::uintptr_t mask =
        static_cast<std::uintptr_t>(alignment - 1);

    for (std::size_t i = 0; i < m_freeSpans.size(); ++i) {
        const JobMemSpan &span = m_freeSpans[i];

        if (span.pageCount() < pageCount)
            continue;

        if (span.firstPageIndex() >
            std::numeric_limits<std::size_t>::max() / m_pageSize)
            continue;

        const std::size_t localOffset =
            span.firstPageIndex() * m_pageSize;

        if (base > std::numeric_limits<std::uintptr_t>::max() - localOffset)
            continue;

        const std::uintptr_t firstAddress = base + localOffset;

        if (firstAddress >
            std::numeric_limits<std::uintptr_t>::max() - mask)
            continue;

        const std::uintptr_t alignedAddress =
            (firstAddress + mask) & ~mask;

        if (alignedAddress < base)
            continue;

        const std::uintptr_t alignedDistance =
            alignedAddress - base;

        if (alignedDistance >
            std::numeric_limits<std::size_t>::max())
            continue;

        const std::size_t alignedOffset =
            static_cast<std::size_t>(alignedDistance);

        // Allocations in this pool must begin on allocator-page boundaries.
        if (alignedOffset % m_pageSize != 0)
            continue;

        const JobMemPage::Index candidate =
            alignedOffset / m_pageSize;

        if (candidate < span.firstPageIndex())
            continue;

        if (candidate >= span.endPageIndex())
            continue;

        if (pageCount > span.endPageIndex() - candidate)
            continue;

        spanIndex = i;
        firstPageIndex = candidate;

        return true;
    }

    return false;
}

void JobPagePool::splitSpan(
    std::size_t spanIndex,
    JobMemPage::Index firstPageIndex,
    std::size_t pageCount)
{
    const JobMemSpan original = m_freeSpans[spanIndex];
    const JobMemPage::Index allocationEnd =
        firstPageIndex + pageCount;

    const bool hasPrefix =
        original.firstPageIndex() < firstPageIndex;

    const bool hasSuffix =
        allocationEnd < original.endPageIndex();

    if (!hasPrefix && !hasSuffix) {
        m_freeSpans.erase(
            m_freeSpans.begin() +
            static_cast<std::ptrdiff_t>(spanIndex));

        return;
    }

    if (hasPrefix && !hasSuffix) {
        m_freeSpans[spanIndex] = makeSpan(
            original.firstPageIndex(),
            firstPageIndex - original.firstPageIndex());

        return;
    }

    if (!hasPrefix && hasSuffix) {
        m_freeSpans[spanIndex] = makeSpan(
            allocationEnd,
            original.endPageIndex() - allocationEnd);

        return;
    }

    JobMemSpan suffix = makeSpan(
        allocationEnd,
        original.endPageIndex() - allocationEnd);

    // Insert first so vector allocation failure leaves the original span
    // untouched.
    m_freeSpans.insert(
        m_freeSpans.begin() +
            static_cast<std::ptrdiff_t>(spanIndex + 1),
        std::move(suffix));

    m_freeSpans[spanIndex] = makeSpan(
        original.firstPageIndex(),
        firstPageIndex - original.firstPageIndex());
}

void JobPagePool::insertFreeSpan(JobMemSpan span)
{
    m_freeSpans.emplace_back(std::move(span));
    coalesceSpans();
}

void JobPagePool::coalesceSpans()
{
    if (m_freeSpans.size() < 2)
        return;

    std::sort(
        m_freeSpans.begin(),
        m_freeSpans.end(),
        [](const JobMemSpan &lhs, const JobMemSpan &rhs) {
            return lhs.firstPageIndex() < rhs.firstPageIndex();
        });

    std::size_t writeIndex = 0;

    for (std::size_t readIndex = 1;
         readIndex < m_freeSpans.size();
         ++readIndex) {
        JobMemSpan &current = m_freeSpans[writeIndex];
        const JobMemSpan &next = m_freeSpans[readIndex];

        if (current.adjacent(next)) {
            current = makeSpan(
                current.firstPageIndex(),
                current.pageCount() + next.pageCount());

            continue;
        }

        ++writeIndex;

        if (writeIndex != readIndex)
            m_freeSpans[writeIndex] = next;
    }

    m_freeSpans.erase(
        m_freeSpans.begin() +
            static_cast<std::ptrdiff_t>(writeIndex + 1),
        m_freeSpans.end());
}

JobMemSpan JobPagePool::makeSpan(
    JobMemPage::Index firstPageIndex,
    std::size_t pageCount)
{
    const std::size_t localFirst =
        firstPageIndex * m_pageSize;

    const std::size_t byteCount =
        pageCount * m_pageSize;

    const std::size_t first =
        m_extent->range().first() + localFirst;

    const JobMemRange range =
        JobMemRange::fromSize(first, byteCount);

    const JobMemSpan::Id id = m_nextSpanId++;

    return JobMemSpan(
        id,
        m_extent->id(),
        firstPageIndex,
        pageCount,
        m_pageSize,
        range);
}

bool JobPagePool::ownsLocked(const void *ptr) const noexcept
{
    return ptr != nullptr &&
           m_extent &&
           m_extent->mapped() &&
           m_extent->contains(ptr);
}

std::size_t JobPagePool::offsetOf(const void *ptr) const noexcept
{
    if (!ownsLocked(ptr))
        return std::numeric_limits<std::size_t>::max();

    return m_extent->offsetOf(ptr);
}

JobMemPage::Index JobPagePool::pageIndexOf(const void *ptr) const noexcept
{
    if (m_pageSize == 0)
        return JobMemPage::kInvalidIndex;

    const std::size_t offset = offsetOf(ptr);

    if (offset == std::numeric_limits<std::size_t>::max())
        return JobMemPage::kInvalidIndex;

    return offset / m_pageSize;
}

void *JobPagePool::ptrAtPage(JobMemPage::Index pageIndex) noexcept
{
    if (!m_extent || !m_extent->mapped() ||
        pageIndex >= m_pages.size())
        return nullptr;

    if (pageIndex >
        std::numeric_limits<std::size_t>::max() / m_pageSize)
        return nullptr;

    const std::size_t offset =
        pageIndex * m_pageSize;

    return m_extent->ptrAt(offset);
}

const void *JobPagePool::ptrAtPage(JobMemPage::Index pageIndex) const noexcept
{
    if (!m_extent || !m_extent->mapped() ||
        pageIndex >= m_pages.size())
        return nullptr;

    if (pageIndex >
        std::numeric_limits<std::size_t>::max() / m_pageSize)
        return nullptr;

    const std::size_t offset =
        pageIndex * m_pageSize;

    return m_extent->ptrAt(offset);
}

JobMemPool::Metrics JobPagePool::metricsLocked() const noexcept
{
    Metrics result{};

    if (!m_extent)
        return result;

    result.capacityBytes = m_extent->size();
    result.allocatedBytes = m_allocatedBytes;

    for (const auto &span : m_freeSpans)
        result.freeBytes += span.size();

    result.allocationCount = m_allocations.size();

    return result;
}

JobPagePool::PageMetrics JobPagePool::pageMetricsLocked() const noexcept
{
    PageMetrics result{};

    result.pageSize = m_pageSize;
    result.pageCount = m_pages.size();
    result.allocatedPages = m_allocatedPages;

    for (const auto &span : m_freeSpans) {
        result.availablePages += span.pageCount();
        result.largestFreeSpanPages =
            std::max(result.largestFreeSpanPages, span.pageCount());
    }

    result.freeSpanCount = m_freeSpans.size();

    return result;
}

void JobPagePool::init(std::size_t size, std::size_t pageSize)
{
    if (size == 0 || pageSize == 0)
        return;

    init(JobMmap::createShared(size), pageSize);
}

void JobPagePool::init(JobMmap::Ptr mmap, std::size_t pageSize)
{
    m_extent.reset();
    m_pageSize = 0;
    m_pages.clear();
    m_freeSpans.clear();
    m_allocations.clear();
    m_allocatedPages = 0;
    m_allocatedBytes = 0;
    m_nextSpanId = 0;

    if (!mmap ||
        !mmap->isValid() ||
        mmap->addr() == nullptr ||
        mmap->mapLength() == 0)
        return;

    const auto &mappedRanges = mmap->mappedRanges();

    if (mappedRanges.size() != 1)
        return;

    const JobMemRange &mapped = mappedRanges.front();

    if (mapped.first() != 0 ||
        mapped.last() != mmap->mapLength())
        return;

    JobMemExtent::Ptr extent = JobMemExtent::createShared(
        std::move(mmap),
        mapped);

    init(std::move(extent), pageSize);
}

void JobPagePool::init(JobMemExtent::Ptr extent, std::size_t pageSize)
{
    m_extent.reset();
    m_pageSize = 0;
    m_pages.clear();
    m_freeSpans.clear();
    m_allocations.clear();
    m_allocatedPages = 0;
    m_allocatedBytes = 0;
    m_nextSpanId = 0;

    if (!extent)
        return;

    if (!validPageGeometry(*extent, pageSize))
        return;

    m_extent = std::move(extent);
    m_pageSize = pageSize;

    buildPages();
}

void JobPagePool::buildPages()
{
    m_pages.clear();
    m_freeSpans.clear();
    m_allocations.clear();
    m_allocatedPages = 0;
    m_allocatedBytes = 0;
    m_nextSpanId = 0;

    if (!m_extent ||
        !m_extent->mapped() ||
        m_pageSize == 0)
        return;

    const std::size_t count =
        m_extent->size() / m_pageSize;

    if (count == 0)
        return;

    m_pages.reserve(count);

    const std::size_t extentFirst =
        m_extent->range().first();

    for (JobMemPage::Index index = 0;
         index < count;
         ++index) {
        const std::size_t localFirst =
            index * m_pageSize;

        const std::size_t first =
            extentFirst + localFirst;

        const JobMemRange range =
            JobMemRange::fromSize(first, m_pageSize);

        m_pages.emplace_back(
            static_cast<JobMemPage::Id>(index),
            m_extent->id(),
            index,
            m_pageSize,
            range);
    }

    m_freeSpans.emplace_back(
        makeSpan(0, count));
}

void JobPagePool::moveFrom(JobPagePool &&other) noexcept
{
    m_extent = std::move(other.m_extent);
    m_pageSize = other.m_pageSize;
    m_pages = std::move(other.m_pages);
    m_freeSpans = std::move(other.m_freeSpans);
    m_allocations = std::move(other.m_allocations);
    m_allocatedPages = other.m_allocatedPages;
    m_allocatedBytes = other.m_allocatedBytes;
    m_nextSpanId = other.m_nextSpanId;

    other.m_extent.reset();
    other.m_pageSize = 0;
    other.m_pages.clear();
    other.m_freeSpans.clear();
    other.m_allocations.clear();
    other.m_allocatedPages = 0;
    other.m_allocatedBytes = 0;
    other.m_nextSpanId = 0;
}

} // namespace job::io