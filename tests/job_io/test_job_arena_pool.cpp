// test_job_arena_pool.cpp

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

#include <job_arena_pool.h>
#include <job_mem_extent.h>
#include <job_mem_pool.h>
#include <job_mem_range.h>
#include <job_mem_page.h>
#include <job_mmap.h>

namespace job::io::test {

[[nodiscard]] std::size_t arenaPoolPageSize() noexcept
{
    return job::io::systemPageSize();
}

[[nodiscard]] std::size_t arenaPoolSize() noexcept
{
    return arenaPoolPageSize() * 256;
}

void requireArenaPoolMetricsInvariant(const JobArenaPool &pool)
{
    const auto metrics = pool.metrics();
    const auto arenaMetrics = pool.arenaMetrics();

    REQUIRE(metrics.capacityBytes == pool.size());
    REQUIRE(metrics.allocatedBytes == pool.allocated());
    REQUIRE(metrics.freeBytes == pool.available());
    REQUIRE(metrics.allocationCount == pool.allocationCount());

    REQUIRE(metrics.allocatedBytes + metrics.freeBytes == metrics.capacityBytes);

    REQUIRE(arenaMetrics.usedBytes == pool.allocated());
    REQUIRE(arenaMetrics.availableBytes == pool.available());
    REQUIRE(arenaMetrics.paddingBytes == pool.padding());
    REQUIRE(arenaMetrics.highWatermarkBytes == pool.highWatermark());

    REQUIRE(arenaMetrics.usedBytes == pool.offset());
    REQUIRE(arenaMetrics.highWatermarkBytes >= arenaMetrics.usedBytes);
    REQUIRE(arenaMetrics.paddingBytes <= arenaMetrics.usedBytes);
}

} // namespace job::io::test

//////////////////////////////////////////////////////////
// Block 1: Usage / examples
//////////////////////////////////////////////////////////

TEST_CASE("JobArenaPool creates a monotonic allocator over anonymous memory",
          "[job_io][arena_pool][usage]")
{
    const std::size_t arenaSize = job::io::test::arenaPoolSize();

    REQUIRE(arenaSize > 0);

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    REQUIRE(pool->type() == job::io::JobMemPool::Type::Arena);

    REQUIRE(pool->extent());
    REQUIRE(pool->mmap());
    REQUIRE(pool->extent()->mapped());

    REQUIRE(pool->size() == arenaSize);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == arenaSize);

    REQUIRE(pool->offset() == 0);
    REQUIRE(pool->padding() == 0);
    REQUIRE(pool->highWatermark() == 0);
    REQUIRE(pool->allocationCount() == 0);

    const auto metrics = pool->metrics();

    REQUIRE(metrics.capacityBytes == arenaSize);
    REQUIRE(metrics.allocatedBytes == 0);
    REQUIRE(metrics.freeBytes == arenaSize);
    REQUIRE(metrics.allocationCount == 0);

    const auto arenaMetrics = pool->arenaMetrics();

    REQUIRE(arenaMetrics.usedBytes == 0);
    REQUIRE(arenaMetrics.availableBytes == arenaSize);
    REQUIRE(arenaMetrics.paddingBytes == 0);
    REQUIRE(arenaMetrics.highWatermarkBytes == 0);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool allocates monotonically",
          "[job_io][arena_pool][usage][allocation]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    auto *first = static_cast<std::byte *>(pool->alloc(64, 1));
    auto *second = static_cast<std::byte *>(pool->alloc(128, 1));
    auto *third = static_cast<std::byte *>(pool->alloc(32, 1));

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);

    REQUIRE(second == first + 64);
    REQUIRE(third == second + 128);

    REQUIRE(pool->offset() == 224);
    REQUIRE(pool->allocated() == 224);
    REQUIRE(pool->available() == 4096 - 224);

    REQUIRE(pool->padding() == 0);
    REQUIRE(pool->allocationCount() == 3);
    REQUIRE(pool->highWatermark() == 224);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool allocations are writable and independent",
          "[job_io][arena_pool][usage][memory]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    auto *first = static_cast<std::uint64_t *>(pool->alloc(sizeof(std::uint64_t), alignof(std::uint64_t)));
    auto *second = static_cast<std::uint64_t *>(pool->alloc(sizeof(std::uint64_t), alignof(std::uint64_t)));

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first != second);

    *first = 0x0123456789ABCDEFULL;
    *second = 0xFEDCBA9876543210ULL;

    REQUIRE(*first == 0x0123456789ABCDEFULL);
    REQUIRE(*second == 0xFEDCBA9876543210ULL);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool honors requested alignment using actual addresses",
          "[job_io][arena_pool][usage][alignment]")
{
    const std::size_t pageSize = job::io::test::arenaPoolPageSize();

    REQUIRE(pageSize > 0);

    const auto pool = job::io::JobArenaPool::createShared(pageSize * 8);

    REQUIRE(pool);

    const std::size_t alignment = pageSize * 2;

    void *ptr = pool->alloc(64, alignment);

    REQUIRE(ptr != nullptr);

    REQUIRE(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);

    REQUIRE(pool->allocated() == pool->padding() + 64);
    REQUIRE(pool->highWatermark() == pool->allocated());

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool accounts for alignment padding as consumed memory",
          "[job_io][arena_pool][usage][alignment][metrics]")
{
    constexpr std::size_t arenaSize = 4096;

    const auto mmap = job::io::JobMmap::createShared(arenaSize);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    auto *base = static_cast<std::byte *>(mmap->addr());

    REQUIRE(base != nullptr);

    const auto pool = job::io::JobArenaPool::createShared(mmap);

    REQUIRE(pool);

    void *first = pool->alloc(1, 1);

    REQUIRE(first == base);
    REQUIRE(pool->allocated() == 1);
    REQUIRE(pool->padding() == 0);

    void *second = pool->alloc(8, 8);

    REQUIRE(second != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(second) % 8 == 0);

    const std::size_t expectedPadding =
        static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(second) - (reinterpret_cast<std::uintptr_t>(base) + 1));

    REQUIRE(pool->padding() == expectedPadding);
    REQUIRE(pool->allocated() == 1 + expectedPadding + 8);
    REQUIRE(pool->available() == pool->size() - pool->allocated());

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}
TEST_CASE("JobArenaPool owns its complete extent domain",
          "[job_io][arena_pool][usage][ownership]")
{
    constexpr std::size_t arenaSize = 4096;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);
    REQUIRE(pool->extent());

    const auto *base = static_cast<const std::byte *>(pool->extent()->addr());

    REQUIRE(base != nullptr);

    REQUIRE(pool->owns(base));
    REQUIRE(pool->owns(base + arenaSize / 2));
    REQUIRE(pool->owns(base + arenaSize - 1));

    REQUIRE_FALSE(pool->owns(base + arenaSize));
}

TEST_CASE("JobArenaPool does not individually reclaim allocations",
          "[job_io][arena_pool][usage][free]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    void *ptr = pool->alloc(64);

    REQUIRE(ptr != nullptr);
    REQUIRE(pool->owns(ptr));

    const std::size_t allocatedBefore = pool->allocated();
    const std::size_t availableBefore = pool->available();
    const std::size_t countBefore = pool->allocationCount();

    REQUIRE_FALSE(pool->free(ptr));

    REQUIRE(pool->allocated() == allocatedBefore);
    REQUIRE(pool->available() == availableBefore);
    REQUIRE(pool->allocationCount() == countBefore);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool clear reclaims the complete arena",
          "[job_io][arena_pool][usage][clear]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    REQUIRE(pool->alloc(123, 1) != nullptr);
    REQUIRE(pool->alloc(64, 64) != nullptr);
    REQUIRE(pool->alloc(321, 16) != nullptr);

    REQUIRE(pool->allocated() > 0);
    REQUIRE(pool->allocationCount() == 3);

    const std::size_t highWatermark = pool->highWatermark();

    REQUIRE(highWatermark == pool->allocated());

    pool->clear();

    REQUIRE(pool->offset() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == pool->size());

    REQUIRE(pool->padding() == 0);
    REQUIRE(pool->allocationCount() == 0);

    // High watermark is lifetime telemetry, not current-epoch state.
    REQUIRE(pool->highWatermark() == highWatermark);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool reuses its beginning after clear",
          "[job_io][arena_pool][usage][clear][reuse]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    void *first = pool->alloc(64, 16);

    REQUIRE(first != nullptr);

    pool->clear();

    void *second = pool->alloc(64, 16);

    REQUIRE(second != nullptr);
    REQUIRE(second == first);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool high watermark survives smaller allocation epochs",
          "[job_io][arena_pool][usage][metrics]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    REQUIRE(pool->alloc(1024, 1) != nullptr);

    REQUIRE(pool->highWatermark() == 1024);

    pool->clear();

    REQUIRE(pool->highWatermark() == 1024);

    REQUIRE(pool->alloc(128, 1) != nullptr);

    REQUIRE(pool->allocated() == 128);
    REQUIRE(pool->highWatermark() == 1024);

    pool->clear();

    REQUIRE(pool->alloc(2048, 1) != nullptr);

    REQUIRE(pool->allocated() == 2048);
    REQUIRE(pool->highWatermark() == 2048);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool uses backing-relative geometry for non-zero extents",
          "[job_io][arena_pool][usage][extent]")
{
    const std::size_t pageSize = job::io::test::arenaPoolPageSize();

    REQUIRE(pageSize > 0);

    auto mmap = job::io::JobMmap::createShared(pageSize * 8);

    REQUIRE(mmap);
    REQUIRE(mmap->isValid());

    const job::io::JobMemRange range(
        pageSize * 2,
        pageSize * 6);

    auto extent = job::io::JobMemExtent::createShared(
        mmap,
        range);

    REQUIRE(extent);
    REQUIRE(extent->size() == pageSize * 4);

    const auto pool = job::io::JobArenaPool::createShared(extent);

    REQUIRE(pool);

    REQUIRE(pool->extent() == extent);
    REQUIRE(pool->mmap() == mmap);
    REQUIRE(pool->size() == pageSize * 4);

    auto *ptr = static_cast<std::byte *>(pool->alloc(64, 1));

    REQUIRE(ptr != nullptr);

    REQUIRE(ptr ==
            static_cast<std::byte *>(mmap->addr()) +
                pageSize * 2);

    REQUIRE(pool->owns(ptr));

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool factories create shared and unique arenas",
          "[job_io][arena_pool][usage][factory]")
{
    constexpr std::size_t arenaSize = 4096;

    const auto shared = job::io::JobArenaPool::createShared(arenaSize);
    const auto unique = job::io::JobArenaPool::createUniq(arenaSize);

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE(shared->type() == job::io::JobMemPool::Type::Arena);
    REQUIRE(unique->type() == job::io::JobMemPool::Type::Arena);

    REQUIRE(shared->size() == arenaSize);
    REQUIRE(unique->size() == arenaSize);

    REQUIRE(shared->extent());
    REQUIRE(unique->extent());

    REQUIRE(shared->extent()->id() != unique->extent()->id());
}

//////////////////////////////////////////////////////////
// Block 2: Edge cases / failure behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobArenaPool rejects zero-sized allocations",
          "[job_io][arena_pool][edge][allocation]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    REQUIRE(pool->alloc(0) == nullptr);

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->allocationCount() == 0);
    REQUIRE(pool->highWatermark() == 0);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool rejects invalid alignment",
          "[job_io][arena_pool][edge][alignment]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    REQUIRE(pool->alloc(64, 0) == nullptr);
    REQUIRE(pool->alloc(64, 3) == nullptr);
    REQUIRE(pool->alloc(64, 6) == nullptr);
    REQUIRE(pool->alloc(64, 12) == nullptr);

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->padding() == 0);
    REQUIRE(pool->allocationCount() == 0);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool rejects allocations larger than remaining capacity",
          "[job_io][arena_pool][edge][capacity]")
{
    constexpr std::size_t arenaSize = 4096;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(arenaSize, 1) != nullptr);

    REQUIRE(pool->allocated() == arenaSize);
    REQUIRE(pool->available() == 0);
    REQUIRE(pool->allocationCount() == 1);

    REQUIRE(pool->alloc(1, 1) == nullptr);

    REQUIRE(pool->allocated() == arenaSize);
    REQUIRE(pool->available() == 0);
    REQUIRE(pool->allocationCount() == 1);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool failed allocation does not consume padding",
          "[job_io][arena_pool][edge][capacity][alignment]")
{
    constexpr std::size_t arenaSize = 64;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    REQUIRE(pool->alloc(63, 1) != nullptr);

    const std::size_t offsetBefore = pool->offset();
    const std::size_t paddingBefore = pool->padding();
    const std::size_t countBefore = pool->allocationCount();
    const std::size_t highWatermarkBefore = pool->highWatermark();

    REQUIRE(pool->alloc(2, 64) == nullptr);

    REQUIRE(pool->offset() == offsetBefore);
    REQUIRE(pool->padding() == paddingBefore);
    REQUIRE(pool->allocationCount() == countBefore);
    REQUIRE(pool->highWatermark() == highWatermarkBefore);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool free always rejects individual reclamation",
          "[job_io][arena_pool][edge][free]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    std::uint64_t foreign = 0;

    REQUIRE_FALSE(pool->free(nullptr));
    REQUIRE_FALSE(pool->free(&foreign));

    void *ptr = pool->alloc(64);

    REQUIRE(ptr != nullptr);

    REQUIRE_FALSE(pool->free(ptr));

    REQUIRE(pool->allocationCount() == 1);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool rejects foreign ownership",
          "[job_io][arena_pool][edge][ownership]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    std::uint64_t foreign = 0;

    REQUIRE_FALSE(pool->owns(nullptr));
    REQUIRE_FALSE(pool->owns(&foreign));
}

TEST_CASE("JobArenaPool zero-size construction creates an empty arena",
          "[job_io][arena_pool][edge][zero]")
{
    const auto pool = job::io::JobArenaPool::createShared(0);

    REQUIRE(pool);

    REQUIRE(pool->type() == job::io::JobMemPool::Type::Arena);

    REQUIRE(pool->extent() == nullptr);
    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->offset() == 0);
    REQUIRE(pool->padding() == 0);
    REQUIRE(pool->highWatermark() == 0);
    REQUIRE(pool->allocationCount() == 0);

    REQUIRE(pool->alloc(1) == nullptr);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool rejects null JobMmap",
          "[job_io][arena_pool][edge][mmap]")
{
    const auto pool = job::io::JobArenaPool::createShared(
        job::io::JobMmap::Ptr{});

    REQUIRE(pool);

    REQUIRE(pool->extent() == nullptr);
    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->alloc(1) == nullptr);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool rejects null JobMemExtent",
          "[job_io][arena_pool][edge][extent]")
{
    const auto pool = job::io::JobArenaPool::createShared(
        job::io::JobMemExtent::Ptr{});

    REQUIRE(pool);

    REQUIRE(pool->extent() == nullptr);
    REQUIRE(pool->mmap() == nullptr);

    REQUIRE(pool->size() == 0);
    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == 0);

    REQUIRE(pool->alloc(1) == nullptr);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool exhausts exact capacity with repeated allocations",
          "[job_io][arena_pool][edge][capacity]")
{
    constexpr std::size_t arenaSize = 4096;
    constexpr std::size_t objectSize = 64;
    constexpr std::size_t objectCount = arenaSize / objectSize;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    std::vector<void *> allocations;
    allocations.reserve(objectCount);

    for (std::size_t i = 0; i < objectCount; ++i) {
        void *ptr = pool->alloc(objectSize, 1);

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->allocated() == arenaSize);
    REQUIRE(pool->available() == 0);
    REQUIRE(pool->allocationCount() == objectCount);

    REQUIRE(pool->alloc(objectSize, 1) == nullptr);

    for (std::size_t i = 1; i < allocations.size(); ++i) {
        REQUIRE(
            static_cast<std::byte *>(allocations[i]) ==
            static_cast<std::byte *>(allocations[i - 1]) +
                objectSize);
    }

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool survives repeated allocation epochs",
          "[job_io][arena_pool][edge][reuse]")
{
    constexpr std::size_t arenaSize = 4096;
    constexpr std::size_t iterations = 10'000;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    for (std::size_t i = 0; i < iterations; ++i) {
        void *ptr = pool->alloc(64, 16);

        REQUIRE(ptr != nullptr);

        REQUIRE(
            reinterpret_cast<std::uintptr_t>(ptr) %
                16 ==
            0);

        pool->clear();
    }

    REQUIRE(pool->allocated() == 0);
    REQUIRE(pool->available() == arenaSize);
    REQUIRE(pool->allocationCount() == 0);

    REQUIRE(pool->highWatermark() >= 64);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool handles many live monotonic allocations",
          "[job_io][arena_pool][edge][many]")
{
    constexpr std::size_t allocationCount = 4096;
    constexpr std::size_t allocationSize = 64;
    constexpr std::size_t arenaSize =
        allocationCount * allocationSize;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    std::vector<void *> allocations;
    allocations.reserve(allocationCount);

    for (std::size_t i = 0; i < allocationCount; ++i) {
        void *ptr = pool->alloc(allocationSize, 1);

        REQUIRE(ptr != nullptr);

        allocations.push_back(ptr);
    }

    REQUIRE(pool->allocationCount() == allocationCount);
    REQUIRE(pool->allocated() == arenaSize);
    REQUIRE(pool->available() == 0);

    for (std::size_t i = 1; i < allocations.size(); ++i) {
        REQUIRE(
            static_cast<std::byte *>(allocations[i]) ==
            static_cast<std::byte *>(allocations[i - 1]) +
                allocationSize);
    }

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool is safe for concurrent callers",
          "[job_io][arena_pool][edge][threads]")
{
    constexpr std::size_t threadCount = 8;
    constexpr std::size_t iterations = 2000;
    constexpr std::size_t allocationSize = 64;

    const std::size_t arenaSize =
        threadCount * iterations * allocationSize;

    const auto pool = job::io::JobArenaPool::createShared(arenaSize);

    REQUIRE(pool);

    std::atomic_bool failed{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (std::size_t thread = 0; thread < threadCount; ++thread) {
        threads.emplace_back([&] {
            for (std::size_t i = 0; i < iterations; ++i) {
                void *ptr = pool->alloc(allocationSize, 1);

                if (ptr == nullptr) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    for (auto &thread : threads)
        thread.join();

    REQUIRE_FALSE(failed.load(std::memory_order_relaxed));

    REQUIRE(pool->allocationCount() == threadCount * iterations);
    REQUIRE(pool->allocated() == arenaSize);
    REQUIRE(pool->available() == 0);

    job::io::test::requireArenaPoolMetricsInvariant(*pool);
}

TEST_CASE("JobArenaPool move construction transfers arena state",
          "[job_io][arena_pool][edge][move]")
{
    job::io::JobArenaPool source(4096);

    REQUIRE(source.extent());

    void *first = source.alloc(64, 1);
    void *second = source.alloc(128, 1);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    const auto extent = source.extent();

    const std::size_t offset = source.offset();
    const std::size_t padding = source.padding();
    const std::size_t highWatermark = source.highWatermark();
    const std::size_t count = source.allocationCount();

    job::io::JobArenaPool destination(std::move(source));

    REQUIRE(destination.extent() == extent);

    REQUIRE(destination.offset() == offset);
    REQUIRE(destination.padding() == padding);
    REQUIRE(destination.highWatermark() == highWatermark);
    REQUIRE(destination.allocationCount() == count);

    REQUIRE(destination.owns(first));
    REQUIRE(destination.owns(second));

    REQUIRE(source.extent() == nullptr);
    REQUIRE(source.size() == 0);
    REQUIRE(source.allocated() == 0);
    REQUIRE(source.available() == 0);
    REQUIRE(source.offset() == 0);
    REQUIRE(source.padding() == 0);
    REQUIRE(source.highWatermark() == 0);
    REQUIRE(source.allocationCount() == 0);

    job::io::test::requireArenaPoolMetricsInvariant(destination);
    job::io::test::requireArenaPoolMetricsInvariant(source);
}

TEST_CASE("JobArenaPool move assignment transfers arena state",
          "[job_io][arena_pool][edge][move]")
{
    job::io::JobArenaPool source(4096);
    job::io::JobArenaPool destination(8192);

    void *ptr = source.alloc(256, 1);

    REQUIRE(ptr != nullptr);

    REQUIRE(destination.alloc(512, 1) != nullptr);

    const auto sourceExtent = source.extent();

    REQUIRE(sourceExtent);

    const std::size_t sourceOffset = source.offset();
    const std::size_t sourceHighWatermark = source.highWatermark();
    const std::size_t sourceCount = source.allocationCount();

    destination = std::move(source);

    REQUIRE(destination.extent() == sourceExtent);
    REQUIRE(destination.size() == 4096);

    REQUIRE(destination.offset() == sourceOffset);
    REQUIRE(destination.highWatermark() == sourceHighWatermark);
    REQUIRE(destination.allocationCount() == sourceCount);

    REQUIRE(destination.owns(ptr));

    REQUIRE(source.extent() == nullptr);
    REQUIRE(source.size() == 0);
    REQUIRE(source.offset() == 0);
    REQUIRE(source.highWatermark() == 0);
    REQUIRE(source.allocationCount() == 0);

    job::io::test::requireArenaPoolMetricsInvariant(destination);
    job::io::test::requireArenaPoolMetricsInvariant(source);
}

//////////////////////////////////////////////////////////
// Block 3: Benchmarks / stress
//////////////////////////////////////////////////////////

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JobArenaPool allocation reuse",
          "[job_io][arena_pool][benchmark]")
{
    const auto pool = job::io::JobArenaPool::createShared(4096);

    REQUIRE(pool);

    BENCHMARK("JobArenaPool alloc/clear 64 bytes")
    {
        pool->clear();

        return pool->alloc(64, 16) != nullptr;
    };
}

TEST_CASE("Benchmark JobArenaPool raw allocation hot path",
          "[job_io][arena_pool][benchmark]")
{
    constexpr std::size_t batchSize = 256;
    constexpr std::size_t allocationSize = 64;

    const auto pool = job::io::JobArenaPool::createShared(
        batchSize * allocationSize * 2);

    REQUIRE(pool);

    BENCHMARK_ADVANCED("JobArenaPool allocate 256 x 64 byte blocks")(Catch::Benchmark::Chronometer meter)
    {
        meter.measure([&] {
            pool->clear();

            for (std::size_t i = 0; i < batchSize; ++i)
                (void)pool->alloc(allocationSize, 16);
        });
    };
}

TEST_CASE("Benchmark JobArenaPool clear",
          "[job_io][arena_pool][benchmark]")
{
    constexpr std::size_t allocationCount = 256;

    const auto pool = job::io::JobArenaPool::createShared(
        allocationCount * 64 * 2);

    REQUIRE(pool);

    BENCHMARK("JobArenaPool clear after 256 allocations")
    {
        pool->clear();

        for (std::size_t i = 0; i < allocationCount; ++i)
            (void)pool->alloc(64, 16);

        pool->clear();

        return pool->allocated();
    };
}

#endif