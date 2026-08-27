// test_job_size_pool.cpp

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

#include <job_mem_pool.h>
#include <job_mem_size.h>
#include <job_page_pool.h>
#include <job_size_pool.h>

namespace job::io::test {

[[nodiscard]] std::size_t sizePoolPageSize() noexcept
{
    return systemPageSize();
}

[[nodiscard]] JobMemSize makeSizeClass(
    std::size_t objectSize = 64,
    std::size_t alignment = 16,
    std::size_t pagesPerSpan = 1)
{
    return JobMemSize(
        1,
        objectSize,
        alignment,
        sizePoolPageSize(),
        pagesPerSpan);
}

[[nodiscard]] JobPagePool::Ptr makeSizePagePool(
    std::size_t pageCount = 256)
{
    const std::size_t pageSize = sizePoolPageSize();

    return JobPagePool::createShared(
        pageSize * pageCount,
        pageSize);
}

[[nodiscard]] JobSizePool::Ptr makeSizePool(
    std::size_t objectSize = 64,
    std::size_t alignment = 16,
    std::size_t pagesPerSpan = 1,
    std::size_t pageCount = 256)
{
    return JobSizePool::createShared(
        makeSizeClass(
            objectSize,
            alignment,
            pagesPerSpan),
        makeSizePagePool(pageCount));
}

void requireSizePoolMetricsInvariant(const JobSizePool &pool)
{
    const auto metrics = pool.metrics();
    const auto sizeMetrics = pool.sizeMetrics();

    REQUIRE(metrics.capacityBytes == pool.size());
    REQUIRE(metrics.allocatedBytes == pool.allocated());
    REQUIRE(metrics.freeBytes == pool.available());

    REQUIRE(metrics.allocatedBytes + metrics.freeBytes ==
            metrics.capacityBytes);

    REQUIRE(sizeMetrics.objectSize == pool.objectSize());
    REQUIRE(sizeMetrics.alignment == pool.alignment());

    REQUIRE(sizeMetrics.objectCapacity ==
            pool.objectCapacity());

    REQUIRE(sizeMetrics.allocatedObjects ==
            pool.allocatedObjects());

    REQUIRE(sizeMetrics.availableObjects ==
            pool.availableObjects());

    REQUIRE(sizeMetrics.allocatedObjects +
                sizeMetrics.availableObjects ==
            sizeMetrics.objectCapacity);

    REQUIRE(metrics.capacityBytes ==
            sizeMetrics.objectCapacity *
                sizeMetrics.objectSize);

    REQUIRE(metrics.allocatedBytes ==
            sizeMetrics.allocatedObjects *
                sizeMetrics.objectSize);

    REQUIRE(metrics.freeBytes ==
            sizeMetrics.availableObjects *
                sizeMetrics.objectSize);

    REQUIRE(metrics.allocationCount ==
            sizeMetrics.allocatedObjects);
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobSizePool creates an empty fixed-size allocator",
          "[job_io][size_pool][usage]")
{
    const auto pagePool =
        job::io::test::makeSizePagePool();

    const auto sizeClass =
        job::io::test::makeSizeClass();

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    REQUIRE(pool->type() ==
            job::io::JobMemPool::Type::Size);

    REQUIRE(pool->pagePool() == pagePool);

    REQUIRE(pool->sizeClass() == sizeClass);

    REQUIRE(pool->objectSize() == 64);
    REQUIRE(pool->alignment() == 16);

    // SizePool grows lazily.
    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->objectCapacity() == 0);
    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->availableObjects() == 0);

    const auto metrics = pool->metrics();

    REQUIRE(metrics.capacityBytes == 0);
    REQUIRE(metrics.allocatedBytes == 0);
    REQUIRE(metrics.freeBytes == 0);
    REQUIRE(metrics.allocationCount == 0);

    const auto sizeMetrics = pool->sizeMetrics();

    REQUIRE(sizeMetrics.objectSize == 64);
    REQUIRE(sizeMetrics.alignment == 16);

    REQUIRE(sizeMetrics.objectCapacity == 0);
    REQUIRE(sizeMetrics.allocatedObjects == 0);
    REQUIRE(sizeMetrics.availableObjects == 0);

    REQUIRE(sizeMetrics.spanCount == 0);
    REQUIRE(sizeMetrics.pagesPerSpan == 1);
    REQUIRE(sizeMetrics.reservedBytes == 0);
    REQUIRE(sizeMetrics.internalWasteBytes == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool grows one configured span on first allocation",
          "[job_io][size_pool][usage][grow]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pagePool =
        job::io::test::makeSizePagePool();

    const auto sizeClass =
        job::io::test::makeSizeClass(
            64,
            16,
            1);

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    REQUIRE(pagePool->allocatedPages() == 0);

    void *ptr = pool->allocObject();

    REQUIRE(ptr != nullptr);

    REQUIRE(pagePool->allocatedPages() == 1);

    REQUIRE(pool->objectCapacity() ==
            sizeClass.objectsPerSpan());

    REQUIRE(pool->allocatedObjects() == 1);

    REQUIRE(pool->availableObjects() ==
            sizeClass.objectsPerSpan() - 1);

    REQUIRE(pool->size() ==
            sizeClass.objectsPerSpan() *
                sizeClass.objectSize());

    REQUIRE(pool->allocated() ==
            sizeClass.objectSize());

    REQUIRE(pool->available() ==
            (sizeClass.objectsPerSpan() - 1) *
                sizeClass.objectSize());

    const auto metrics = pool->sizeMetrics();

    REQUIRE(metrics.spanCount == 1);
    REQUIRE(metrics.reservedBytes ==
            sizeClass.spanSize());

    REQUIRE(metrics.internalWasteBytes ==
            sizeClass.wastePerSpan());

    job::io::test::requireSizePoolMetricsInvariant(*pool);

    REQUIRE(pool->free(ptr));
}

TEST_CASE("JobSizePool generic allocation consumes one size-class object",
          "[job_io][size_pool][usage][allocation]")
{
    const auto pool =
        job::io::test::makeSizePool();

    REQUIRE(pool);

    void *a = pool->alloc(1, 1);
    void *b = pool->alloc(32, 8);
    void *c = pool->alloc(64, 16);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);

    REQUIRE(pool->allocatedObjects() == 3);
    REQUIRE(pool->allocated() == 3 * 64);
    REQUIRE(pool->metrics().allocationCount == 3);

    REQUIRE(pool->owns(a));
    REQUIRE(pool->owns(b));
    REQUIRE(pool->owns(c));

    job::io::test::requireSizePoolMetricsInvariant(*pool);

    REQUIRE(pool->free(a));
    REQUIRE(pool->free(b));
    REQUIRE(pool->free(c));

    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->allocated() == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool objects satisfy the configured alignment",
          "[job_io][size_pool][usage][alignment]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            32);

    REQUIRE(pool);

    std::vector<void *> allocations;

    constexpr std::size_t allocationCount = 64;

    allocations.reserve(allocationCount);

    for (std::size_t i = 0;
         i < allocationCount;
         ++i) {
        void *ptr = pool->allocObject();

        REQUIRE(ptr != nullptr);

        REQUIRE(
            reinterpret_cast<std::uintptr_t>(ptr) %
                pool->alignment() ==
            0);

        allocations.push_back(ptr);
    }

    for (void *ptr : allocations)
        REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool objects are writable and independent",
          "[job_io][size_pool][usage][memory]")
{
    struct TestObject final
    {
        std::uint64_t a;
        std::uint64_t b;
        std::uint64_t c;
        std::uint64_t d;
    };

    static_assert(sizeof(TestObject) == 32);

    const auto pool =
        job::io::test::makeSizePool(
            sizeof(TestObject),
            alignof(TestObject));

    REQUIRE(pool);

    auto *first =
        static_cast<TestObject *>(pool->allocObject());

    auto *second =
        static_cast<TestObject *>(pool->allocObject());

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first != second);

    first->a = 11;
    first->b = 22;
    first->c = 33;
    first->d = 44;

    second->a = 55;
    second->b = 66;
    second->c = 77;
    second->d = 88;

    REQUIRE(first->a == 11);
    REQUIRE(first->b == 22);
    REQUIRE(first->c == 33);
    REQUIRE(first->d == 44);

    REQUIRE(second->a == 55);
    REQUIRE(second->b == 66);
    REQUIRE(second->c == 77);
    REQUIRE(second->d == 88);

    REQUIRE(pool->free(first));
    REQUIRE(pool->free(second));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool reuses released objects",
          "[job_io][size_pool][usage][reuse]")
{
    const auto pool =
        job::io::test::makeSizePool();

    REQUIRE(pool);

    void *first = pool->allocObject();

    REQUIRE(first != nullptr);

    REQUIRE(pool->free(first));

    void *second = pool->allocObject();

    REQUIRE(second != nullptr);
    REQUIRE(second == first);

    REQUIRE(pool->free(second));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool grows another span when all current objects are live",
          "[job_io][size_pool][usage][grow]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pagePool =
        job::io::test::makeSizePagePool();

    const auto sizeClass =
        job::io::test::makeSizeClass(
            64,
            16,
            1);

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    const std::size_t objectsPerSpan =
        sizeClass.objectsPerSpan();

    REQUIRE(objectsPerSpan > 0);

    std::vector<void *> allocations;
    allocations.reserve(objectsPerSpan + 1);

    for (std::size_t i = 0;
         i < objectsPerSpan;
         ++i) {
        void *ptr = pool->allocObject();

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->sizeMetrics().spanCount == 1);
    REQUIRE(pagePool->allocatedPages() == 1);

    void *next = pool->allocObject();

    REQUIRE(next != nullptr);

    allocations.push_back(next);

    REQUIRE(pool->sizeMetrics().spanCount == 2);
    REQUIRE(pagePool->allocatedPages() == 2);

    REQUIRE(pool->objectCapacity() ==
            objectsPerSpan * 2);

    for (void *ptr : allocations)
        REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool supports objects larger than one allocator page",
          "[job_io][size_pool][usage][span]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    const std::size_t objectSize =
        pageSize * 2;

    const auto pagePool =
        job::io::test::makeSizePagePool(32);

    const job::io::JobMemSize sizeClass(
        7,
        objectSize,
        pageSize,
        pageSize,
        2);

    REQUIRE_FALSE(sizeClass.fitsInPage());
    REQUIRE(sizeClass.fitsInSpan());

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    void *ptr = pool->allocObject();

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->allocatedObjects() == 1);
    REQUIRE(pool->objectCapacity() == 1);

    REQUIRE(pagePool->allocatedPages() == 2);

    REQUIRE(pool->sizeMetrics().spanCount == 1);
    REQUIRE(pool->sizeMetrics().pagesPerSpan == 2);
    REQUIRE(pool->sizeMetrics().reservedBytes ==
            pageSize * 2);

    REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool reports span waste separately from object capacity",
          "[job_io][size_pool][usage][metrics]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    constexpr std::size_t objectSize = 96;

    const auto pagePool =
        job::io::test::makeSizePagePool();

    const job::io::JobMemSize sizeClass(
        8,
        objectSize,
        32,
        pageSize,
        1);

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    void *ptr = pool->allocObject();

    REQUIRE(ptr != nullptr);

    const auto metrics = pool->sizeMetrics();

    REQUIRE(metrics.objectCapacity ==
            sizeClass.objectsPerSpan());

    REQUIRE(metrics.reservedBytes ==
            sizeClass.spanSize());

    REQUIRE(metrics.internalWasteBytes ==
            sizeClass.wastePerSpan());

    REQUIRE(
        metrics.objectCapacity *
                sizeClass.objectSize() +
            metrics.internalWasteBytes ==
        metrics.reservedBytes);

    REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool clear returns all reserved spans to JobPagePool",
          "[job_io][size_pool][usage][clear]")
{
    const auto pagePool =
        job::io::test::makeSizePagePool();

    const auto pool =
        job::io::JobSizePool::createShared(
            job::io::test::makeSizeClass(
                64,
                16,
                2),
            pagePool);

    REQUIRE(pool);

    REQUIRE(pagePool->allocatedPages() == 0);

    REQUIRE(pool->allocObject() != nullptr);

    REQUIRE(pagePool->allocatedPages() == 2);

    pool->clear();

    REQUIRE(pool->objectCapacity() == 0);
    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->availableObjects() == 0);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->sizeMetrics().spanCount == 0);

    REQUIRE(pagePool->allocatedPages() == 0);
    REQUIRE(pagePool->availablePages() ==
            pagePool->pageCount());

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool destruction returns reserved spans to JobPagePool",
          "[job_io][size_pool][usage][lifetime]")
{
    const auto pagePool =
        job::io::test::makeSizePagePool();

    REQUIRE(pagePool);
    REQUIRE(pagePool->allocatedPages() == 0);

    {
        const auto pool =
            job::io::JobSizePool::createShared(
                job::io::test::makeSizeClass(
                    64,
                    16,
                    2),
                pagePool);

        REQUIRE(pool);

        REQUIRE(pool->allocObject() != nullptr);

        REQUIRE(pagePool->allocatedPages() == 2);
    }

    REQUIRE(pagePool->allocatedPages() == 0);
    REQUIRE(pagePool->availablePages() ==
            pagePool->pageCount());
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobSizePool rejects invalid generic allocation requests",
          "[job_io][size_pool][edge][allocation]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            16);

    REQUIRE(pool);

    REQUIRE(pool->alloc(0) == nullptr);

    REQUIRE(pool->alloc(65) == nullptr);

    REQUIRE(pool->alloc(32, 0) == nullptr);
    REQUIRE(pool->alloc(32, 3) == nullptr);
    REQUIRE(pool->alloc(32, 6) == nullptr);

    REQUIRE(pool->alloc(32, 32) == nullptr);

    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->objectCapacity() == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool rejects incompatible JobPagePool page geometry",
          "[job_io][size_pool][edge][geometry]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pagePool =
        job::io::JobPagePool::createShared(
            pageSize * 16,
            pageSize);

    REQUIRE(pagePool);

    const job::io::JobMemSize sizeClass(
        2,
        64,
        16,
        pageSize * 2,
        1);

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    REQUIRE(pool->pagePool() == nullptr);

    REQUIRE(pool->allocObject() == nullptr);

    REQUIRE(pool->objectCapacity() == 0);
    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->availableObjects() == 0);
}

TEST_CASE("JobSizePool rejects null JobPagePool",
          "[job_io][size_pool][edge][page_pool]")
{
    const auto sizeClass =
        job::io::test::makeSizeClass();

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            job::io::JobPagePool::Ptr{});

    REQUIRE(pool);

    REQUIRE(pool->pagePool() == nullptr);

    REQUIRE(pool->allocObject() == nullptr);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->objectCapacity() == 0);
}

TEST_CASE("JobSizePool rejects pointers it does not own",
          "[job_io][size_pool][edge][ownership]")
{
    const auto pool =
        job::io::test::makeSizePool();

    REQUIRE(pool);

    std::uint64_t foreign = 0;

    REQUIRE_FALSE(pool->owns(&foreign));
    REQUIRE_FALSE(pool->free(&foreign));
}

TEST_CASE("JobSizePool owns reserved span addresses but only frees object starts",
          "[job_io][size_pool][edge][ownership]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            16);

    REQUIRE(pool);

    auto *ptr =
        static_cast<std::byte *>(pool->allocObject());

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->owns(ptr));
    REQUIRE(pool->owns(ptr + 1));

    REQUIRE_FALSE(pool->free(ptr + 1));

    REQUIRE(pool->allocatedObjects() == 1);

    REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool only frees live object allocations",
          "[job_io][size_pool][edge][free]")
{
    const auto pool =
        job::io::test::makeSizePool();

    REQUIRE(pool);

    void *ptr = pool->allocObject();

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->free(ptr));
    REQUIRE_FALSE(pool->free(ptr));

    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool rejects free of another object boundary inside a live span",
          "[job_io][size_pool][edge][free]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            16);

    REQUIRE(pool);

    auto *ptr =
        static_cast<std::byte *>(pool->allocObject());

    REQUIRE(ptr != nullptr);

    // The first grow creates many valid object boundaries. This address
    // belongs to the SizePool and is a valid slot boundary, but it is not
    // currently a live allocation.
    auto *otherSlot =
        ptr - static_cast<std::ptrdiff_t>(pool->objectSize());

    if (pool->owns(otherSlot))
        REQUIRE_FALSE(pool->free(otherSlot));

    REQUIRE(pool->allocatedObjects() == 1);

    REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool exhausts its backing PagePool cleanly",
          "[job_io][size_pool][edge][capacity]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    constexpr std::size_t backingPages = 2;

    const auto pagePool =
        job::io::JobPagePool::createShared(
            pageSize * backingPages,
            pageSize);

    REQUIRE(pagePool);

    const auto sizeClass =
        job::io::test::makeSizeClass(
            64,
            16,
            1);

    const auto pool =
        job::io::JobSizePool::createShared(
            sizeClass,
            pagePool);

    REQUIRE(pool);

    const std::size_t capacity =
        sizeClass.objectsPerSpan() *
        backingPages;

    std::vector<void *> allocations;
    allocations.reserve(capacity);

    for (std::size_t i = 0;
         i < capacity;
         ++i) {
        void *ptr = pool->allocObject();

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->allocatedObjects() == capacity);
    REQUIRE(pool->availableObjects() == 0);

    REQUIRE(pagePool->allocatedPages() ==
            backingPages);

    REQUIRE(pool->allocObject() == nullptr);

    for (void *ptr : allocations)
        REQUIRE(pool->free(ptr));

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool survives repeated allocation and reuse",
          "[job_io][size_pool][edge][reuse]")
{
    const auto pool =
        job::io::test::makeSizePool();

    REQUIRE(pool);

    constexpr std::size_t iterations = 10'000;

    for (std::size_t i = 0;
         i < iterations;
         ++i) {
        void *ptr = pool->allocObject();

        REQUIRE(ptr != nullptr);

        REQUIRE(
            reinterpret_cast<std::uintptr_t>(ptr) %
                pool->alignment() ==
            0);

        REQUIRE(pool->free(ptr));
    }

    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool handles many simultaneously live objects",
          "[job_io][size_pool][edge][many]")
{
    const auto pool =
        job::io::test::makeSizePool();

    REQUIRE(pool);

    constexpr std::size_t allocationCount = 4096;

    std::vector<void *> allocations;
    allocations.reserve(allocationCount);

    for (std::size_t i = 0;
         i < allocationCount;
         ++i) {
        void *ptr = pool->allocObject();

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->allocatedObjects() ==
            allocationCount);

    REQUIRE(pool->metrics().allocationCount ==
            allocationCount);

    job::io::test::requireSizePoolMetricsInvariant(*pool);

    for (std::size_t i = 0;
         i < allocations.size();
         i += 2)
        REQUIRE(pool->free(allocations[i]));

    REQUIRE(pool->allocatedObjects() ==
            allocationCount / 2);

    job::io::test::requireSizePoolMetricsInvariant(*pool);

    for (std::size_t i = 1;
         i < allocations.size();
         i += 2)
        REQUIRE(pool->free(allocations[i]));

    REQUIRE(pool->allocatedObjects() == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool is safe for concurrent callers",
          "[job_io][size_pool][edge][threads]")
{
    constexpr std::size_t threadCount = 8;
    constexpr std::size_t iterations = 2000;

    const auto pool =
        job::io::test::makeSizePool(
            64,
            16,
            1,
            256);

    REQUIRE(pool);

    std::atomic_bool failed{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (std::size_t thread = 0;
         thread < threadCount;
         ++thread) {
        threads.emplace_back([&] {
            for (std::size_t i = 0;
                 i < iterations;
                 ++i) {
                void *ptr = pool->allocObject();

                if (ptr == nullptr) {
                    failed.store(
                        true,
                        std::memory_order_relaxed);

                    return;
                }

                if (reinterpret_cast<std::uintptr_t>(ptr) %
                        pool->alignment() !=
                    0) {
                    failed.store(
                        true,
                        std::memory_order_relaxed);

                    return;
                }

                if (!pool->free(ptr)) {
                    failed.store(
                        true,
                        std::memory_order_relaxed);

                    return;
                }
            }
        });
    }

    for (auto &thread : threads)
        thread.join();

    REQUIRE_FALSE(
        failed.load(
            std::memory_order_relaxed));

    REQUIRE(pool->allocatedObjects() == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    job::io::test::requireSizePoolMetricsInvariant(*pool);
}

TEST_CASE("JobSizePool move construction transfers reserved spans and allocations",
          "[job_io][size_pool][edge][move]")
{
    const auto pagePool =
        job::io::test::makeSizePagePool();

    const auto sizeClass =
        job::io::test::makeSizeClass(
            64,
            16,
            2);

    job::io::JobSizePool source(
        sizeClass,
        pagePool);

    void *first = source.allocObject();
    void *second = source.allocObject();

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    REQUIRE(pagePool->allocatedPages() == 2);
    REQUIRE(source.allocatedObjects() == 2);

    job::io::JobSizePool destination(
        std::move(source));

    REQUIRE(destination.pagePool() == pagePool);

    REQUIRE(destination.sizeClass() == sizeClass);

    REQUIRE(destination.allocatedObjects() == 2);
    REQUIRE(destination.metrics().allocationCount == 2);

    REQUIRE(source.pagePool() == nullptr);
    REQUIRE(source.objectCapacity() == 0);
    REQUIRE(source.allocatedObjects() == 0);
    REQUIRE(source.availableObjects() == 0);

    REQUIRE(destination.free(first));
    REQUIRE(destination.free(second));

    destination.clear();

    REQUIRE(pagePool->allocatedPages() == 0);
}

TEST_CASE("JobSizePool move assignment releases old spans and transfers new state",
          "[job_io][size_pool][edge][move]")
{
    const auto sourcePagePool =
        job::io::test::makeSizePagePool();

    const auto destinationPagePool =
        job::io::test::makeSizePagePool();

    const auto sizeClass =
        job::io::test::makeSizeClass(
            64,
            16,
            2);

    job::io::JobSizePool source(sizeClass,sourcePagePool);
    job::io::JobSizePool destination(sizeClass, destinationPagePool);

    void *sourcePtr = source.allocObject();
    void *destinationPtr = destination.allocObject();

    REQUIRE(sourcePtr != nullptr);
    REQUIRE(destinationPtr != nullptr);

    REQUIRE(sourcePagePool->allocatedPages() == 2);
    REQUIRE(destinationPagePool->allocatedPages() == 2);

    destination = std::move(source);

    // Destination's previous backing allocation was returned before
    // adopting source's state.
    REQUIRE(destinationPagePool->allocatedPages() == 0);

    REQUIRE(destination.pagePool() ==
            sourcePagePool);

    REQUIRE(destination.allocatedObjects() == 1);
    REQUIRE(destination.metrics().allocationCount == 1);

    REQUIRE(source.pagePool() == nullptr);
    REQUIRE(source.objectCapacity() == 0);
    REQUIRE(source.allocatedObjects() == 0);

    REQUIRE(destination.free(sourcePtr));

    destination.clear();

    REQUIRE(sourcePagePool->allocatedPages() == 0);
}

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JobSizePool object allocation reuse",
          "[job_io][size_pool][benchmark]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            16,
            1,
            256);

    REQUIRE(pool);

    // Pre-grow the pool so this benchmark measures the steady-state
    // fixed-size allocation path rather than PagePool growth.
    void *warmup = pool->allocObject();

    REQUIRE(warmup != nullptr);
    REQUIRE(pool->free(warmup));

    BENCHMARK("JobSizePool alloc/free 64 byte object")
    {
        void *ptr = pool->allocObject();

        if (ptr == nullptr)
            return false;

        return pool->free(ptr);
    };
}

TEST_CASE("Benchmark JobSizePool generic allocation reuse",
          "[job_io][size_pool][benchmark]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            16,
            1,
            256);

    REQUIRE(pool);

    void *warmup = pool->alloc(64, 16);

    REQUIRE(warmup != nullptr);
    REQUIRE(pool->free(warmup));

    BENCHMARK("JobSizePool alloc/free 64 byte request")
    {
        void *ptr = pool->alloc(64, 16);

        if (ptr == nullptr)
            return false;

        return pool->free(ptr);
    };
}

TEST_CASE("Benchmark JobSizePool live allocation set",
          "[job_io][size_pool][benchmark]")
{
    const auto pool =
        job::io::test::makeSizePool(
            64,
            16,
            1,
            256);

    REQUIRE(pool);

    constexpr std::size_t batchSize = 256;

    // Make sure benchmark timing does not include the first span creation.
    void *warmup = pool->allocObject();

    REQUIRE(warmup != nullptr);
    REQUIRE(pool->free(warmup));

    BENCHMARK_ADVANCED("JobSizePool allocate/free 256 live objects")(Catch::Benchmark::Chronometer meter)
    {
        std::vector<void *> allocations(batchSize);

        meter.measure([&] {
            for (std::size_t i = 0;
                 i < batchSize;
                 ++i)
                allocations[i] =
                    pool->allocObject();

            for (void *ptr : allocations)
                (void)pool->free(ptr);
        });
    };
}

TEST_CASE("Benchmark JobSizePool span growth",
          "[job_io][size_pool][benchmark]")
{
    const std::size_t pageSize =
        job::io::test::sizePoolPageSize();

    REQUIRE(pageSize > 0);

    BENCHMARK_ADVANCED("JobSizePool create, grow one span, destroy")(Catch::Benchmark::Chronometer meter)
    {
        meter.measure([&] {
            auto pagePool =
                job::io::JobPagePool::createShared(
                    pageSize * 256,
                    pageSize);

            const job::io::JobMemSize sizeClass(
                99,
                64,
                16,
                pageSize,
                1);

            auto pool =
                job::io::JobSizePool::createShared(
                    sizeClass,
                    pagePool);

            void *ptr = pool->allocObject();

            return ptr != nullptr;
        });
    };
}

#endif