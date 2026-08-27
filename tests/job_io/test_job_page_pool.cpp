// test_job_page_pool.cpp

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <job_mem_extent.h>
#include <job_mem_page.h>
#include <job_mem_pool.h>
#include <job_mem_range.h>
#include <job_page_pool.h>

namespace job::io::test {

[[nodiscard]] std::size_t pagePoolPageSize() noexcept
{
    return systemPageSize();
}

[[nodiscard]] std::size_t pagePoolSize() noexcept
{
    return pagePoolPageSize() * 256;
}

void requirePagePoolMetricsInvariant(const JobPagePool &pool)
{
    const auto metrics = pool.metrics();
    const auto pageMetrics = pool.pageMetrics();

    REQUIRE(metrics.capacityBytes == pool.size());
    REQUIRE(metrics.allocatedBytes == pool.allocated());
    REQUIRE(metrics.freeBytes == pool.available());
    REQUIRE(metrics.allocatedBytes + metrics.freeBytes == metrics.capacityBytes);

    REQUIRE(pageMetrics.pageSize == pool.pageSize());
    REQUIRE(pageMetrics.pageCount == pool.pageCount());
    REQUIRE(pageMetrics.allocatedPages == pool.allocatedPages());
    REQUIRE(pageMetrics.availablePages == pool.availablePages());

    REQUIRE(pageMetrics.allocatedPages + pageMetrics.availablePages == pageMetrics.pageCount);

    REQUIRE(metrics.allocatedBytes == pageMetrics.allocatedPages * pageMetrics.pageSize);
    REQUIRE(metrics.freeBytes == pageMetrics.availablePages * pageMetrics.pageSize);
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobPagePool creates a page allocator over anonymous memory",
          "[job_io][page_pool][usage]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    const std::size_t poolSize = job::io::test::pagePoolSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(poolSize, pageSize);

    REQUIRE(pool);
    REQUIRE(pool->type() == job::io::JobMemPool::Type::Page);

    REQUIRE(pool->extent());
    REQUIRE(pool->mmap());
    REQUIRE(pool->extent()->mapped());

    REQUIRE(pool->pageSize() == pageSize);
    REQUIRE(pool->pageCount() == poolSize / pageSize);

    REQUIRE(pool->size() == poolSize);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == poolSize);

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == pool->pageCount());

    const auto metrics = pool->metrics();

    REQUIRE(metrics.capacityBytes == poolSize);
    REQUIRE(metrics.allocatedBytes == 0);
    REQUIRE(metrics.freeBytes == poolSize);
    REQUIRE(metrics.allocationCount == 0);

    const auto pageMetrics = pool->pageMetrics();

    REQUIRE(pageMetrics.pageSize == pageSize);
    REQUIRE(pageMetrics.pageCount == poolSize / pageSize);
    REQUIRE(pageMetrics.allocatedPages == 0);
    REQUIRE(pageMetrics.availablePages == poolSize / pageSize);
    REQUIRE(pageMetrics.freeSpanCount == 1);
    REQUIRE(pageMetrics.largestFreeSpanPages == poolSize / pageSize);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool builds page descriptors from its extent",
          "[job_io][page_pool][usage][pages]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    constexpr std::size_t pageCount = 16;

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * pageCount, pageSize);

    REQUIRE(pool);

    const auto extent = pool->extent();

    REQUIRE(extent);

    const auto &pages = pool->pages();

    REQUIRE(pages.size() == pageCount);

    for (std::size_t index = 0; index < pages.size(); ++index) {
        const auto &page = pages[index];

        REQUIRE(page.id() == index);
        REQUIRE(page.extentId() == extent->id());
        REQUIRE(page.index() == index);
        REQUIRE(page.pageSize() == pageSize);

        const std::size_t expectedFirst = extent->range().first() + index * pageSize;

        REQUIRE(page.first() == expectedFirst);
        REQUIRE(page.last() == expectedFirst + pageSize);
        REQUIRE(page.size() == pageSize);

        REQUIRE(extent->contains(page.range()));
    }

    for (std::size_t index = 1; index < pages.size(); ++index)
        REQUIRE(pages[index - 1].adjacent(pages[index]));
}

TEST_CASE("JobPagePool allocates complete allocator pages for byte requests",
          "[job_io][page_pool][usage][allocation]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 16, pageSize);

    REQUIRE(pool);

    void *a = pool->alloc(1);

    REQUIRE(a != nullptr);
    REQUIRE(pool->owns(a));

    REQUIRE(pool->allocatedPages() == 1);
    REQUIRE(pool->allocated() == pageSize);

    void *b = pool->alloc(pageSize);

    REQUIRE(b != nullptr);

    REQUIRE(pool->allocatedPages() == 2);
    REQUIRE(pool->allocated() == pageSize * 2);

    void *c = pool->alloc(pageSize + 1);

    REQUIRE(c != nullptr);

    REQUIRE(pool->allocatedPages() == 4);
    REQUIRE(pool->allocated() == pageSize * 4);

    REQUIRE(pool->metrics().allocationCount == 3);

    job::io::test::requirePagePoolMetricsInvariant(*pool);

    REQUIRE(pool->free(a));
    REQUIRE(pool->free(b));
    REQUIRE(pool->free(c));

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == pool->size());

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pool->pageCount());

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool allocPages allocates contiguous page runs",
          "[job_io][page_pool][usage][pages][allocation]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    constexpr std::size_t pageCount = 32;

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * pageCount, pageSize);

    REQUIRE(pool);

    auto *first = static_cast<std::byte *>(pool->allocPages(3));
    auto *second = static_cast<std::byte *>(pool->allocPages(5));

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    REQUIRE(second == first + pageSize * 3);

    REQUIRE(pool->allocatedPages() == 8);
    REQUIRE(pool->allocated() == pageSize * 8);
    REQUIRE(pool->metrics().allocationCount == 2);

    job::io::test::requirePagePoolMetricsInvariant(*pool);

    REQUIRE(pool->free(first));
    REQUIRE(pool->free(second));

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->pageMetrics().freeSpanCount == 1);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool allocations are writable",
          "[job_io][page_pool][usage][memory]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 16, pageSize);

    REQUIRE(pool);

    constexpr std::size_t count = 1024;

    auto *data = static_cast<std::uint32_t *>(
        pool->alloc(count * sizeof(std::uint32_t), alignof(std::uint32_t)));

    REQUIRE(data != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(data) % alignof(std::uint32_t) == 0);

    for (std::size_t i = 0; i < count; ++i)
        data[i] = static_cast<std::uint32_t>(i * 7);

    for (std::size_t i = 0; i < count; ++i)
        REQUIRE(data[i] == static_cast<std::uint32_t>(i * 7));

    REQUIRE(pool->free(data));

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool honors allocation alignment at page boundaries",
          "[job_io][page_pool][usage][alignment]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 64, pageSize);

    REQUIRE(pool);

    const std::size_t alignment = pageSize * 2;

    void *ptr = pool->alloc(1, alignment);

    REQUIRE(ptr != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    REQUIRE(pool->free(ptr));

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool can allocate its complete page domain",
          "[job_io][page_pool][usage][capacity]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    constexpr std::size_t pageCount = 8;

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * pageCount, pageSize);

    REQUIRE(pool);

    void *ptr = pool->allocPages(pageCount);

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->allocatedPages() == pageCount);
    REQUIRE(pool->availablePages() == 0);

    REQUIRE(pool->allocated() == pageSize * pageCount);
    REQUIRE(pool->available() == 0);

    const auto pageMetrics = pool->pageMetrics();

    REQUIRE(pageMetrics.freeSpanCount == 0);
    REQUIRE(pageMetrics.largestFreeSpanPages == 0);

    REQUIRE(pool->allocPages(1) == nullptr);

    job::io::test::requirePagePoolMetricsInvariant(*pool);

    REQUIRE(pool->free(ptr));

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == pageCount);

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pageCount);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool reuses released page runs",
          "[job_io][page_pool][usage][reuse]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 16, pageSize);

    REQUIRE(pool);

    void *first = pool->allocPages(3);

    REQUIRE(first != nullptr);
    REQUIRE(pool->free(first));

    void *second = pool->allocPages(3);

    REQUIRE(second != nullptr);
    REQUIRE(second == first);

    REQUIRE(pool->free(second));

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool coalesces adjacent released spans",
          "[job_io][page_pool][usage][coalesce]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 16, pageSize);

    REQUIRE(pool);

    void *a = pool->allocPages(2);
    void *b = pool->allocPages(2);
    void *c = pool->allocPages(2);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);

    REQUIRE(pool->free(b));

    REQUIRE(pool->pageMetrics().freeSpanCount == 2);

    REQUIRE(pool->free(a));

    // [a][b] is now one span while c remains allocated.
    REQUIRE(pool->pageMetrics().freeSpanCount == 2);

    REQUIRE(pool->free(c));

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pool->pageCount());

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool coalesces released spans on both sides",
          "[job_io][page_pool][usage][coalesce]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 16, pageSize);

    REQUIRE(pool);

    void *a = pool->allocPages(1);
    void *b = pool->allocPages(1);
    void *c = pool->allocPages(1);
    void *d = pool->allocPages(1);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);
    REQUIRE(d != nullptr);

    REQUIRE(pool->free(a));
    REQUIRE(pool->free(c));

    REQUIRE(pool->pageMetrics().freeSpanCount == 3);

    // Freeing b joins [a][b][c].
    REQUIRE(pool->free(b));

    REQUIRE(pool->pageMetrics().freeSpanCount == 2);

    REQUIRE(pool->free(d));

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pool->pageCount());

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool clear restores one complete free span",
          "[job_io][page_pool][usage][clear]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 16, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(1) != nullptr);
    REQUIRE(pool->allocPages(2) != nullptr);
    REQUIRE(pool->alloc(pageSize + 1) != nullptr);

    REQUIRE(pool->allocatedPages() == 5);
    REQUIRE(pool->metrics().allocationCount == 3);

    pool->clear();

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->allocated() == 0);

    REQUIRE(pool->availablePages() == pool->pageCount());
    REQUIRE(pool->available() == pool->size());

    REQUIRE(pool->metrics().allocationCount == 0);

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pool->pageCount());

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}
TEST_CASE("JobPagePool uses backing-relative page geometry for non-zero extents",
          "[job_io][page_pool][usage][extent]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    auto mmap = job::io::JobMmap::createShared(pageSize * 8);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemRange extentRange(
        pageSize * 2,
        pageSize * 6);

    auto extent = job::io::JobMemExtent::createShared(
        mmap,
        extentRange);

    REQUIRE(extent);
    REQUIRE(extent->size() == pageSize * 4);

    const auto extentId = extent->id();

    REQUIRE(extentId != job::io::JobMemExtent::kInvalidId);

    const auto pool = job::io::JobPagePool::createShared(extent, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->extent() == extent);
    REQUIRE(pool->mmap() == mmap);

    REQUIRE(pool->pageCount() == 4);
    REQUIRE(pool->pageSize() == pageSize);

    const auto &pages = pool->pages();

    REQUIRE(pages.size() == 4);

    REQUIRE(pages[0].extentId() == extentId);
    REQUIRE(pages[0].index() == 0);
    REQUIRE(pages[0].range() == job::io::JobMemRange(pageSize * 2, pageSize * 3));

    REQUIRE(pages[1].extentId() == extentId);
    REQUIRE(pages[1].index() == 1);
    REQUIRE(pages[1].range() == job::io::JobMemRange(pageSize * 3, pageSize * 4));

    auto *ptr = static_cast<std::byte *>(pool->allocPages(1));

    REQUIRE(ptr != nullptr);
    REQUIRE(ptr == static_cast<std::byte *>(mmap->addr()) + pageSize * 2);

    REQUIRE(pool->free(ptr));

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool factories create shared and unique pools",
          "[job_io][page_pool][usage][factory]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    const std::size_t poolSize = pageSize * 8;

    REQUIRE(pageSize > 0);

    const auto shared = job::io::JobPagePool::createShared(poolSize, pageSize);
    const auto unique = job::io::JobPagePool::createUniq(poolSize, pageSize);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->type() == job::io::JobMemPool::Type::Page);
    REQUIRE(unique->type() == job::io::JobMemPool::Type::Page);

    REQUIRE(shared->pageCount() == 8);
    REQUIRE(unique->pageCount() == 8);

    REQUIRE(shared->size() == poolSize);
    REQUIRE(unique->size() == poolSize);
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobPagePool rejects zero-sized allocations",
          "[job_io][page_pool][edge][allocation]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 8, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(0) == nullptr);
    REQUIRE(pool->allocPages(0) == nullptr);

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->allocated() == 0);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool rejects invalid allocation alignment",
          "[job_io][page_pool][edge][alignment]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 8, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(1, 0) == nullptr);
    REQUIRE(pool->alloc(1, 3) == nullptr);
    REQUIRE(pool->alloc(1, 6) == nullptr);
    REQUIRE(pool->alloc(1, 12) == nullptr);

    REQUIRE(pool->allocatedPages() == 0);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool rejects requests larger than available page capacity",
          "[job_io][page_pool][edge][capacity]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    constexpr std::size_t pageCount = 8;

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * pageCount, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->allocPages(pageCount + 1) == nullptr);
    REQUIRE(pool->alloc(pageSize * pageCount + 1) == nullptr);

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == pageCount);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool rejects double free",
          "[job_io][page_pool][edge][free]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 8, pageSize);

    REQUIRE(pool);

    void *ptr = pool->allocPages(2);

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->free(ptr));
    REQUIRE_FALSE(pool->free(ptr));

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool owns extent addresses but only frees allocation starts",
          "[job_io][page_pool][edge][ownership]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 8, pageSize);

    REQUIRE(pool);

    auto *ptr = static_cast<std::byte *>(pool->allocPages(2));

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->owns(ptr));
    REQUIRE(pool->owns(ptr + 1));
    REQUIRE(pool->owns(ptr + pageSize));
    REQUIRE(pool->owns(ptr + pageSize + 1));

    REQUIRE_FALSE(pool->free(ptr + 1));

    // The second page is page-aligned but it is still not the beginning
    // of this two-page allocation.
    REQUIRE_FALSE(pool->free(ptr + pageSize));

    REQUIRE(pool->allocatedPages() == 2);
    REQUIRE(pool->metrics().allocationCount == 1);

    REQUIRE(pool->free(ptr));

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool rejects foreign pointers",
          "[job_io][page_pool][edge][ownership]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 8, pageSize);

    REQUIRE(pool);

    std::uint64_t foreign = 0;

    REQUIRE_FALSE(pool->owns(&foreign));
    REQUIRE_FALSE(pool->free(&foreign));

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool owns the first extent byte but not one-past the extent",
          "[job_io][page_pool][edge][ownership]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    constexpr std::size_t pageCount = 8;

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * pageCount, pageSize);

    REQUIRE(pool);
    REQUIRE(pool->extent());

    const auto *base = static_cast<const std::byte *>(pool->extent()->addr());

    REQUIRE(base != nullptr);

    REQUIRE(pool->owns(base));
    REQUIRE(pool->owns(base + pageSize * pageCount - 1));
    REQUIRE_FALSE(pool->owns(base + pageSize * pageCount));
}

TEST_CASE("JobPagePool zero-size construction produces an empty pool",
          "[job_io][page_pool][edge][zero]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(0, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->type() == job::io::JobMemPool::Type::Page);

    REQUIRE(pool->extent() == nullptr);
    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->pageSize() == 0);
    REQUIRE(pool->pageCount() == 0);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == 0);

    REQUIRE(pool->pages().empty());

    REQUIRE(pool->metrics().capacityBytes == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    REQUIRE(pool->pageMetrics().freeSpanCount == 0);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == 0);

    REQUIRE(pool->alloc(1) == nullptr);
    REQUIRE(pool->allocPages(1) == nullptr);
}

TEST_CASE("JobPagePool rejects a null JobMmap",
          "[job_io][page_pool][edge][mmap]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(
        job::io::JobMmap::Ptr{},
        pageSize);

    REQUIRE(pool);

    REQUIRE(pool->extent() == nullptr);
    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->pageSize() == 0);
    REQUIRE(pool->pageCount() == 0);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->alloc(1) == nullptr);
    REQUIRE(pool->allocPages(1) == nullptr);
}

TEST_CASE("JobPagePool requires complete allocator pages",
          "[job_io][page_pool][edge][geometry]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    auto mmap = job::io::JobMmap::createShared(pageSize * 4);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    auto extent = job::io::JobMemExtent::createShared(
        mmap,
        job::io::JobMemRange(0, pageSize * 3 + 1));

    REQUIRE(extent);
    REQUIRE(extent->id() != job::io::JobMemExtent::kInvalidId);

    const auto pool = job::io::JobPagePool::createShared(extent, pageSize);

    REQUIRE(pool);

    REQUIRE(pool->extent() == nullptr);
    REQUIRE(pool->pageCount() == 0);
    REQUIRE(pool->size() == 0);
}

TEST_CASE("JobPagePool survives repeated page allocation and reuse",
          "[job_io][page_pool][edge][reuse]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 32, pageSize);

    REQUIRE(pool);

    constexpr std::size_t iterations = 10'000;

    for (std::size_t i = 0; i < iterations; ++i) {
        void *ptr = pool->allocPages(1);

        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<std::uintptr_t>(ptr) % pageSize == 0);
        REQUIRE(pool->free(ptr));
    }

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == pool->pageCount());

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pool->pageCount());

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool handles many simultaneously live page allocations",
          "[job_io][page_pool][edge][many]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    constexpr std::size_t pageCount = 256;

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * pageCount, pageSize);

    REQUIRE(pool);

    std::vector<void *> allocations;
    allocations.reserve(pageCount);

    for (std::size_t i = 0; i < pageCount; ++i) {
        void *ptr = pool->allocPages(1);

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->allocatedPages() == pageCount);
    REQUIRE(pool->availablePages() == 0);
    REQUIRE(pool->metrics().allocationCount == pageCount);

    job::io::test::requirePagePoolMetricsInvariant(*pool);

    for (std::size_t i = 0; i < allocations.size(); i += 2)
        REQUIRE(pool->free(allocations[i]));

    REQUIRE(pool->allocatedPages() == pageCount / 2);

    job::io::test::requirePagePoolMetricsInvariant(*pool);

    for (std::size_t i = 1; i < allocations.size(); i += 2)
        REQUIRE(pool->free(allocations[i]));

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == pageCount);

    REQUIRE(pool->pageMetrics().freeSpanCount == 1);
    REQUIRE(pool->pageMetrics().largestFreeSpanPages == pageCount);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool is safe for concurrent callers",
          "[job_io][page_pool][edge][threads]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    constexpr std::size_t threadCount = 8;
    constexpr std::size_t iterations = 2000;

    const auto pool = job::io::JobPagePool::createShared(pageSize * 256, pageSize);

    REQUIRE(pool);

    std::atomic_bool failed{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (std::size_t thread = 0; thread < threadCount; ++thread) {
        threads.emplace_back([&] {
            for (std::size_t i = 0; i < iterations; ++i) {
                void *ptr = pool->allocPages(1);

                if (ptr == nullptr) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }

                if (reinterpret_cast<std::uintptr_t>(ptr) % pageSize != 0) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }

                if (!pool->free(ptr)) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    for (auto &thread : threads)
        thread.join();

    REQUIRE_FALSE(failed.load(std::memory_order_relaxed));

    REQUIRE(pool->allocatedPages() == 0);
    REQUIRE(pool->availablePages() == pool->pageCount());

    REQUIRE(pool->metrics().allocationCount == 0);
    REQUIRE(pool->pageMetrics().freeSpanCount == 1);

    job::io::test::requirePagePoolMetricsInvariant(*pool);
}

TEST_CASE("JobPagePool move construction transfers allocator state",
          "[job_io][page_pool][edge][move]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    job::io::JobPagePool source(pageSize * 16, pageSize);

    void *first = source.allocPages(2);
    void *second = source.allocPages(3);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    const auto extent = source.extent();

    REQUIRE(extent);
    REQUIRE(source.allocatedPages() == 5);

    job::io::JobPagePool destination(std::move(source));

    REQUIRE(destination.extent() == extent);
    REQUIRE(destination.pageSize() == pageSize);
    REQUIRE(destination.pageCount() == 16);

    REQUIRE(destination.allocatedPages() == 5);
    REQUIRE(destination.allocated() == pageSize * 5);
    REQUIRE(destination.metrics().allocationCount == 2);

    REQUIRE(source.extent() == nullptr);
    REQUIRE(source.pageSize() == 0);
    REQUIRE(source.pageCount() == 0);
    REQUIRE(source.allocatedPages() == 0);

    REQUIRE(destination.free(first));
    REQUIRE(destination.free(second));

    REQUIRE(destination.allocatedPages() == 0);
    REQUIRE(destination.availablePages() == 16);

    job::io::test::requirePagePoolMetricsInvariant(destination);
}

TEST_CASE("JobPagePool move assignment transfers allocator state",
          "[job_io][page_pool][edge][move]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    job::io::JobPagePool source(pageSize * 16, pageSize);
    job::io::JobPagePool destination(pageSize * 8, pageSize);

    void *ptr = source.allocPages(4);

    REQUIRE(ptr != nullptr);

    const auto extent = source.extent();

    REQUIRE(extent);

    destination = std::move(source);

    REQUIRE(destination.extent() == extent);
    REQUIRE(destination.pageSize() == pageSize);
    REQUIRE(destination.pageCount() == 16);

    REQUIRE(destination.allocatedPages() == 4);
    REQUIRE(destination.allocated() == pageSize * 4);
    REQUIRE(destination.metrics().allocationCount == 1);

    REQUIRE(source.extent() == nullptr);
    REQUIRE(source.pageSize() == 0);
    REQUIRE(source.pageCount() == 0);

    REQUIRE(destination.free(ptr));

    REQUIRE(destination.allocatedPages() == 0);
    REQUIRE(destination.availablePages() == 16);

    job::io::test::requirePagePoolMetricsInvariant(destination);
}

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JobPagePool page allocation reuse", "[job_io][page_pool][benchmark]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 256, pageSize);
    REQUIRE(pool);

    BENCHMARK("JobPagePool alloc/free one page") {
        void *ptr = pool->allocPages(1);

        if (ptr == nullptr)
            return false;

        return pool->free(ptr);
    };
}

TEST_CASE("Benchmark JobPagePool byte allocation reuse", "[job_io][page_pool][benchmark]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();
    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobPagePool::createShared(pageSize * 256, pageSize);
    REQUIRE(pool);

    BENCHMARK("JobPagePool alloc/free 64 byte request"){
        void *ptr = pool->alloc(64, 16);
        if (ptr == nullptr)
            return false;

        return pool->free(ptr);
    };
}

TEST_CASE("Benchmark JobPagePool live page allocation set", "[job_io][page_pool][benchmark]")
{
    const std::size_t pageSize = job::io::test::pagePoolPageSize();

    REQUIRE(pageSize > 0);

    constexpr std::size_t batchSize = 128;

    const auto pool = job::io::JobPagePool::createShared(pageSize * 256, pageSize);

    REQUIRE(pool);

    BENCHMARK_ADVANCED("JobPagePool allocate/free 128 live pages")(Catch::Benchmark::Chronometer meter){
        std::vector<void *> allocations(batchSize);
        meter.measure([&] {
            for (std::size_t i = 0; i < batchSize; ++i)
                allocations[i] = pool->allocPages(1);

            for (void *ptr : allocations)
                (void)pool->free(ptr);
        });
    };
}

#endif