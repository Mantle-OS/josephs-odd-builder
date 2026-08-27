// test_job_range_pool.cpp

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <job_mmap.h>
#include <job_range_pool.h>
#include <job_mem_page.h>
#include <job_tmp_file.h>

namespace job::io::test {

constexpr std::size_t kRangePoolSize = 1024 * 1024;

[[nodiscard]] std::filesystem::path rangePoolTestPath(const char *name)
{
    return std::filesystem::temp_directory_path() /
           ("job_range_pool_" + std::to_string(::getpid()) + "_" + name + ".tmp");
}

[[nodiscard]] JobMmap::Ptr makeRangePoolMmap(const std::filesystem::path &path)
{
    auto mmap = JobMmap::createShared(path);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());
    REQUIRE(mmap->addr() != nullptr);
    REQUIRE(mmap->mappedSize() == kRangePoolSize);
    REQUIRE(mmap->mappedRanges().size() == 1);
    REQUIRE(mmap->mappedRanges().front() == JobMemRange(0, kRangePoolSize));

    return mmap;
}

[[nodiscard]] JobRangePool::Ptr makeRangePool(const std::filesystem::path &path)
{
    auto pool = JobRangePool::createShared(makeRangePoolMmap(path));

    REQUIRE(pool);

    return pool;
}

void requireMetricsInvariant(const JobRangePool &pool)
{
    const auto metrics = pool.metrics();

    REQUIRE(metrics.allocatedBytes + metrics.freeBytes == metrics.capacityBytes);
    REQUIRE(metrics.capacityBytes == pool.size());
    REQUIRE(metrics.allocatedBytes == pool.allocated());
    REQUIRE(metrics.freeBytes == pool.available());
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobRangePool creates an anonymous range pool", "[job_io][range_pool][usage]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);
    REQUIRE(pool->type() == job::io::JobMemPool::Type::Range);

    REQUIRE(pool->mmap());
    REQUIRE(pool->mmap()->anonymous());
    REQUIRE(pool->mmap()->isValid());

    REQUIRE(pool->size() == job::io::test::kRangePoolSize);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    const auto metrics = pool->metrics();

    REQUIRE(metrics.capacityBytes == job::io::test::kRangePoolSize);
    REQUIRE(metrics.allocatedBytes == 0);
    REQUIRE(metrics.freeBytes == job::io::test::kRangePoolSize);
    REQUIRE(metrics.allocationCount == 0);

    const auto rangeMetrics = pool->rangeMetrics();

    REQUIRE(rangeMetrics.largestFreeBlock == job::io::test::kRangePoolSize);
    REQUIRE(rangeMetrics.freeRangeCount == 1);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool creates a range pool over JobMmap", "[job_io][range_pool][usage][mmap]")
{
    const auto path = job::io::test::rangePoolTestPath("mmap");
    job::io::JobTmpFile tmp(path, job::io::test::kRangePoolSize, std::byte{0x00});

    const auto mmap = job::io::test::makeRangePoolMmap(path);
    const auto pool = job::io::JobRangePool::createShared(mmap);

    REQUIRE(pool);
    REQUIRE(pool->type() == job::io::JobMemPool::Type::Range);

    REQUIRE(pool->mmap() == mmap);

    REQUIRE(pool->size() == job::io::test::kRangePoolSize);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool allocates and frees memory", "[job_io][range_pool][usage][allocation]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    void *ptr = pool->alloc(256);

    REQUIRE(ptr != nullptr);
    REQUIRE(pool->owns(ptr));

    REQUIRE(pool->allocated() == 256);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize - 256);

    const auto metrics = pool->metrics();

    REQUIRE(metrics.allocationCount == 1);
    REQUIRE(metrics.allocatedBytes == 256);

    job::io::test::requireMetricsInvariant(*pool);

    REQUIRE(pool->free(ptr));

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    const auto afterFree = pool->metrics();
    const auto rangeMetrics = pool->rangeMetrics();

    REQUIRE(afterFree.allocationCount == 0);
    REQUIRE(rangeMetrics.freeRangeCount == 1);
    REQUIRE(rangeMetrics.largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool allocations expose writable mapped memory", "[job_io][range_pool][usage][memory]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    constexpr std::size_t count = 1024;

    auto *data = static_cast<std::uint32_t *>(
        pool->alloc(count * sizeof(std::uint32_t), alignof(std::uint32_t)));

    REQUIRE(data != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(data) % alignof(std::uint32_t) == 0);

    for (std::size_t i = 0; i < count; ++i)
        data[i] = static_cast<std::uint32_t>(i * 3);

    for (std::size_t i = 0; i < count; ++i)
        REQUIRE(data[i] == static_cast<std::uint32_t>(i * 3));

    REQUIRE(pool->free(data));

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool honors allocation alignment", "[job_io][range_pool][usage][alignment]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    constexpr std::size_t alignments[] = {
        1,
        2,
        4,
        8,
        16,
        32,
        64,
        128,
        256,
        512,
        1024,
        2048,
        4096,
        8192,
        16384
    };

    std::vector<void *> allocations;

    for (const std::size_t alignment : alignments) {
        void *ptr = pool->alloc(37, alignment);

        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

        allocations.push_back(ptr);
    }

    for (void *ptr : allocations)
        REQUIRE(pool->free(ptr));

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool alignment uses the actual mapped address", "[job_io][range_pool][usage][alignment]")
{
    constexpr std::size_t alignment = 64 * 1024;

    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);
    REQUIRE(pool->mmap());
    REQUIRE(pool->mmap()->addr() != nullptr);

    void *ptr = pool->alloc(1, alignment);

    REQUIRE(ptr != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    REQUIRE(pool->free(ptr));

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool reuses released memory", "[job_io][range_pool][usage][reuse]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    void *first = pool->alloc(256, 16);

    REQUIRE(first != nullptr);
    REQUIRE(pool->free(first));

    void *second = pool->alloc(256, 16);

    REQUIRE(second != nullptr);
    REQUIRE(second == first);

    REQUIRE(pool->free(second));

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool coalesces adjacent released ranges", "[job_io][range_pool][usage][coalesce]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    void *a = pool->alloc(256, 1);
    void *b = pool->alloc(256, 1);
    void *c = pool->alloc(256, 1);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);

    REQUIRE(pool->metrics().allocationCount == 3);

    REQUIRE(pool->free(b));

    REQUIRE(pool->rangeMetrics().freeRangeCount == 2);

    REQUIRE(pool->free(a));

    // a + b are now one free range while c remains allocated.
    REQUIRE(pool->rangeMetrics().freeRangeCount == 2);

    REQUIRE(pool->free(c));

    const auto rangeMetrics = pool->rangeMetrics();

    REQUIRE(rangeMetrics.freeRangeCount == 1);
    REQUIRE(rangeMetrics.largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool coalesces free ranges on both sides", "[job_io][range_pool][usage][coalesce]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    void *a = pool->alloc(128, 1);
    void *b = pool->alloc(128, 1);
    void *c = pool->alloc(128, 1);
    void *d = pool->alloc(128, 1);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);
    REQUIRE(d != nullptr);

    REQUIRE(pool->free(a));
    REQUIRE(pool->free(c));

    REQUIRE(pool->rangeMetrics().freeRangeCount == 3);

    // Freeing b joins [a][b][c] into one range.
    REQUIRE(pool->free(b));

    REQUIRE(pool->rangeMetrics().freeRangeCount == 2);

    REQUIRE(pool->free(d));

    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool clear releases all allocations", "[job_io][range_pool][usage][clear]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(64) != nullptr);
    REQUIRE(pool->alloc(128) != nullptr);
    REQUIRE(pool->alloc(256) != nullptr);

    REQUIRE(pool->allocated() == 448);
    REQUIRE(pool->metrics().allocationCount == 3);

    pool->clear();

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    REQUIRE(pool->metrics().allocationCount == 0);
    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool factories create shared and unique pools", "[job_io][range_pool][usage][factory]")
{
    const auto shared = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);
    const auto unique = job::io::JobRangePool::createUniq(job::io::test::kRangePoolSize);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->type() == job::io::JobMemPool::Type::Range);
    REQUIRE(unique->type() == job::io::JobMemPool::Type::Range);

    REQUIRE(shared->size() == job::io::test::kRangePoolSize);
    REQUIRE(unique->size() == job::io::test::kRangePoolSize);
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobRangePool rejects invalid allocations", "[job_io][range_pool][edge][allocation]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(0) == nullptr);

    REQUIRE(pool->alloc(32, 0) == nullptr);
    REQUIRE(pool->alloc(32, 3) == nullptr);
    REQUIRE(pool->alloc(32, 6) == nullptr);
    REQUIRE(pool->alloc(32, 12) == nullptr);

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool rejects allocations larger than the pool", "[job_io][range_pool][edge][capacity]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(job::io::test::kRangePoolSize + 1) == nullptr);

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool can exactly consume its complete range", "[job_io][range_pool][edge][capacity]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    void *ptr = pool->alloc(job::io::test::kRangePoolSize, 1);

    REQUIRE(ptr != nullptr);

    REQUIRE(pool->allocated() == job::io::test::kRangePoolSize);
    REQUIRE(pool->available() == 0);
    REQUIRE(pool->alloc(1) == nullptr);

    const auto rangeMetrics = pool->rangeMetrics();

    REQUIRE(rangeMetrics.freeRangeCount == 0);
    REQUIRE(rangeMetrics.largestFreeBlock == 0);

    job::io::test::requireMetricsInvariant(*pool);

    REQUIRE(pool->free(ptr));

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool rejects double free", "[job_io][range_pool][edge][free]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    void *ptr = pool->alloc(256);

    REQUIRE(ptr != nullptr);
    REQUIRE(pool->free(ptr));
    REQUIRE_FALSE(pool->free(ptr));

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool rejects pointers outside its mapping", "[job_io][range_pool][edge][ownership]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    std::uint64_t foreign = 0;

    REQUIRE_FALSE(pool->owns(&foreign));
    REQUIRE_FALSE(pool->free(&foreign));

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool owns mapped addresses but only frees allocation starts", "[job_io][range_pool][edge][ownership]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    auto *ptr = static_cast<std::byte *>(pool->alloc(256));

    REQUIRE(ptr != nullptr);

    // owns() asks whether the address belongs to the mapped backing.
    // free() asks whether the address begins a live allocation.
    REQUIRE(pool->owns(ptr));
    REQUIRE(pool->owns(ptr + 32));

    REQUIRE_FALSE(pool->free(ptr + 32));

    REQUIRE(pool->allocated() == 256);

    REQUIRE(pool->free(ptr));

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool owns the first byte but not one-past the mapping", "[job_io][range_pool][edge][ownership]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);
    REQUIRE(pool->mmap());

    const auto *base = static_cast<const std::byte *>(pool->mmap()->addr());

    REQUIRE(base != nullptr);

    REQUIRE(pool->owns(base));
    REQUIRE(pool->owns(base + job::io::test::kRangePoolSize - 1));
    REQUIRE_FALSE(pool->owns(base + job::io::test::kRangePoolSize));
}

TEST_CASE("JobRangePool zero size produces an empty pool", "[job_io][range_pool][edge][zero]")
{
    const auto pool = job::io::JobRangePool::createShared(0);

    REQUIRE(pool);

    REQUIRE(pool->type() == job::io::JobMemPool::Type::Range);

    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->metrics().capacityBytes == 0);
    REQUIRE(pool->metrics().allocatedBytes == 0);
    REQUIRE(pool->metrics().freeBytes == 0);
    REQUIRE(pool->metrics().allocationCount == 0);

    REQUIRE(pool->rangeMetrics().freeRangeCount == 0);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == 0);

    REQUIRE(pool->alloc(1) == nullptr);
}

TEST_CASE("JobRangePool rejects a null JobMmap", "[job_io][range_pool][edge][mmap]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::JobMmap::Ptr{});

    REQUIRE(pool);

    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->alloc(64) == nullptr);
}

TEST_CASE("JobRangePool rejects a fragmented JobMmap", "[job_io][range_pool][edge][mmap][fragmentation]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    auto mmap = job::io::JobMmap::createShared(pageSize * 3);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    REQUIRE(mmap->unmap(job::io::JobMemRange(pageSize, pageSize * 2)));
    REQUIRE(mmap->mappedRanges().size() == 2);

    const auto pool = job::io::JobRangePool::createShared(mmap);

    REQUIRE(pool);

    REQUIRE(pool->mmap() == nullptr);
    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->alloc(64) == nullptr);
}

TEST_CASE("JobRangePool ownership follows currently mapped JobMmap ranges", "[job_io][range_pool][edge][mmap][fragmentation]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    auto mmap = job::io::JobMmap::createShared(pageSize * 3);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const auto pool = job::io::JobRangePool::createShared(mmap);

    REQUIRE(pool);
    REQUIRE(pool->mmap() == mmap);

    const auto *base = static_cast<const std::byte *>(mmap->addr());

    REQUIRE(pool->owns(base));
    REQUIRE(pool->owns(base + pageSize));
    REQUIRE(pool->owns(base + pageSize * 2));

    REQUIRE(mmap->unmap(job::io::JobMemRange(pageSize, pageSize * 2)));

    REQUIRE(mmap->mappedRanges().size() == 2);

    REQUIRE(pool->owns(base));
    REQUIRE_FALSE(pool->owns(base + pageSize));
    REQUIRE(pool->owns(base + pageSize * 2));
}

TEST_CASE("JobRangePool clear does not invent memory across a fragmented mapping", "[job_io][range_pool][edge][clear][fragmentation]")
{
    const std::size_t pageSize = job::io::systemPageSize();
    REQUIRE(pageSize > 0);

    auto mmap = job::io::JobMmap::createShared(pageSize * 3);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const auto pool = job::io::JobRangePool::createShared(mmap);

    REQUIRE(pool);

    void *ptr = pool->alloc(64);

    REQUIRE(ptr != nullptr);

    REQUIRE(mmap->unmap(job::io::JobMemRange(pageSize, pageSize * 2)));
    REQUIRE(mmap->mappedRanges().size() == 2);

    pool->clear();

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->rangeMetrics().freeRangeCount == 0);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == 0);

    REQUIRE(pool->alloc(64) == nullptr);
}

TEST_CASE("JobRangePool survives repeated allocation and reuse", "[job_io][range_pool][edge][reuse]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    constexpr std::size_t iterations = 10'000;

    for (std::size_t i = 0; i < iterations; ++i) {
        void *ptr = pool->alloc(64, 16);

        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<std::uintptr_t>(ptr) % 16 == 0);
        REQUIRE(pool->free(ptr));
    }

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool handles many simultaneously live allocations", "[job_io][range_pool][edge][many]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    constexpr std::size_t allocationSize = 64;
    constexpr std::size_t allocationCount = 4096;

    std::vector<void *> allocations;
    allocations.reserve(allocationCount);

    for (std::size_t i = 0; i < allocationCount; ++i) {
        void *ptr = pool->alloc(allocationSize, 16);

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->metrics().allocationCount == allocationCount);
    REQUIRE(pool->allocated() == allocationSize * allocationCount);

    job::io::test::requireMetricsInvariant(*pool);

    for (std::size_t i = 0; i < allocations.size(); i += 2)
        REQUIRE(pool->free(allocations[i]));

    job::io::test::requireMetricsInvariant(*pool);

    for (std::size_t i = 1; i < allocations.size(); i += 2)
        REQUIRE(pool->free(allocations[i]));

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool is safe for concurrent callers", "[job_io][range_pool][edge][threads]")
{
    constexpr std::size_t threadCount = 8;
    constexpr std::size_t iterations = 2000;
    constexpr std::size_t allocationSize = 64;

    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    std::atomic_bool failed{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (std::size_t thread = 0; thread < threadCount; ++thread) {
        threads.emplace_back([&] {
            for (std::size_t i = 0; i < iterations; ++i) {
                void *ptr = pool->alloc(allocationSize, 16);

                if (ptr == nullptr) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }

                if (reinterpret_cast<std::uintptr_t>(ptr) % 16 != 0) {
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

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == job::io::test::kRangePoolSize);

    REQUIRE(pool->metrics().allocationCount == 0);
    REQUIRE(pool->rangeMetrics().freeRangeCount == 1);
    REQUIRE(pool->rangeMetrics().largestFreeBlock == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(*pool);
}

TEST_CASE("JobRangePool move construction transfers allocator state", "[job_io][range_pool][edge][move]")
{
    job::io::JobRangePool source(job::io::test::kRangePoolSize);

    void *first = source.alloc(128, 16);
    void *second = source.alloc(256, 16);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    const auto mmap = source.mmap();

    REQUIRE(mmap);
    REQUIRE(source.allocated() == 384);

    job::io::JobRangePool destination(std::move(source));

    REQUIRE(destination.mmap() == mmap);
    REQUIRE(destination.size() == job::io::test::kRangePoolSize);
    REQUIRE(destination.allocated() == 384);
    REQUIRE(destination.metrics().allocationCount == 2);

    REQUIRE(source.mmap() == nullptr);
    REQUIRE(source.size() == 0);
    REQUIRE(source.allocated() == 0);
    REQUIRE(source.available() == 0);

    REQUIRE(destination.free(first));
    REQUIRE(destination.free(second));

    REQUIRE(destination.allocated() == 0);
    REQUIRE(destination.available() == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(destination);
}

TEST_CASE("JobRangePool move assignment transfers allocator state", "[job_io][range_pool][edge][move]")
{
    job::io::JobRangePool source(job::io::test::kRangePoolSize);
    job::io::JobRangePool destination(job::io::test::kRangePoolSize / 2);

    void *ptr = source.alloc(512, 64);

    REQUIRE(ptr != nullptr);

    const auto mmap = source.mmap();

    REQUIRE(mmap);

    destination = std::move(source);

    REQUIRE(destination.mmap() == mmap);
    REQUIRE(destination.size() == job::io::test::kRangePoolSize);
    REQUIRE(destination.allocated() == 512);
    REQUIRE(destination.metrics().allocationCount == 1);

    REQUIRE(source.mmap() == nullptr);
    REQUIRE(source.size() == 0);
    REQUIRE(source.allocated() == 0);

    REQUIRE(destination.free(ptr));

    REQUIRE(destination.allocated() == 0);
    REQUIRE(destination.available() == job::io::test::kRangePoolSize);

    job::io::test::requireMetricsInvariant(destination);
}

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JobRangePool allocation reuse", "[job_io][range_pool][benchmark]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);

    REQUIRE(pool);

    BENCHMARK("JobRangePool alloc/free 64 bytes")
    {
        void *ptr = pool->alloc(64, 16);

        if (ptr == nullptr)
            return false;

        return pool->free(ptr);
    };
}

TEST_CASE("Benchmark JobRangePool live allocation set", "[job_io][range_pool][benchmark]")
{
    const auto pool = job::io::JobRangePool::createShared(job::io::test::kRangePoolSize);
    REQUIRE(pool);

    constexpr std::size_t batchSize = 256;

    BENCHMARK_ADVANCED("JobRangePool allocate/free 256 live blocks")(Catch::Benchmark::Chronometer meter) {
        std::vector<void *> allocations(batchSize);
        meter.measure([&] {
            for (std::size_t i = 0; i < batchSize; ++i)
                allocations[i] = pool->alloc(64, 16);

            for (void *ptr : allocations)
                (void)pool->free(ptr);
        });
    };
}

#endif