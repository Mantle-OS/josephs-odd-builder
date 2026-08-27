#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

#include "job_mem_extent.h"
#include "job_mem_pool.h"
#include "job_mmap.h"

namespace job::io {

// Monotonic linear allocator.
//
// JobArenaPool allocates aligned byte ranges by advancing a single offset.
// Individual allocations are not returned to the arena. The complete arena
// is reclaimed with clear().
//
// Allocation:
//
//     [ used ][ padding ][ allocation ][ free ... ]
//                     ^
//                     aligned address
//
// This makes JobArenaPool useful for workloads where many temporary
// allocations share the same lifetime.
//
// JobArenaPool owns a JobMemExtent which defines its complete allocation
// domain. The extent may begin at a non-zero JobMmap-relative offset.
//
// allocated() reports all consumed arena bytes, including alignment padding.
// This preserves:
//
//     allocated() + available() == size()
//
// padding() reports how much of the consumed space was alignment padding.
//
// highWatermark() records the greatest number of simultaneously consumed
// arena bytes and is intentionally not reset by clear().
class JobArenaPool final : public JobMemPool
{
public:
    using Ptr  = std::shared_ptr<JobArenaPool>;
    using WPtr = std::weak_ptr<JobArenaPool>;
    using UPtr = std::unique_ptr<JobArenaPool>;

    struct ArenaMetrics final
    {
        std::size_t usedBytes{0};
        std::size_t availableBytes{0};
        std::size_t paddingBytes{0};
        std::size_t highWatermarkBytes{0};
    };

    explicit JobArenaPool(std::size_t size);
    explicit JobArenaPool(JobMmap::Ptr mmap);
    explicit JobArenaPool(JobMemExtent::Ptr extent);

    ~JobArenaPool() override;

    JobArenaPool(const JobArenaPool &) = delete;
    JobArenaPool &operator=(const JobArenaPool &) = delete;

    JobArenaPool(JobArenaPool &&other) noexcept;
    JobArenaPool &operator=(JobArenaPool &&other) noexcept;

    [[nodiscard]] static Ptr createShared(std::size_t size);
    [[nodiscard]] static Ptr createShared(JobMmap::Ptr mmap);
    [[nodiscard]] static Ptr createShared(JobMemExtent::Ptr extent);

    [[nodiscard]] static UPtr createUniq(std::size_t size);
    [[nodiscard]] static UPtr createUniq(JobMmap::Ptr mmap);
    [[nodiscard]] static UPtr createUniq(JobMemExtent::Ptr extent);

    [[nodiscard]] Type type() const noexcept override final;

    [[nodiscard]] void *alloc(
        std::size_t size,
        std::size_t alignment = alignof(std::max_align_t)) override final;

    // Arena allocations share the arena lifetime. Individual reclamation is
    // unsupported; use clear() to reclaim the complete arena.
    [[nodiscard]] bool free(void *ptr) override final;

    [[nodiscard]] bool owns(const void *ptr) const noexcept override final;

    [[nodiscard]] std::size_t size() const noexcept override final;
    [[nodiscard]] std::size_t allocated() const noexcept override final;
    [[nodiscard]] std::size_t available() const noexcept override final;

    [[nodiscard]] Metrics metrics() const noexcept override final;
    [[nodiscard]] ArenaMetrics arenaMetrics() const noexcept;

    void clear() override final;

    [[nodiscard]] std::size_t offset() const noexcept;
    [[nodiscard]] std::size_t padding() const noexcept;
    [[nodiscard]] std::size_t highWatermark() const noexcept;
    [[nodiscard]] std::size_t allocationCount() const noexcept;

    [[nodiscard]] JobMemExtent::Ptr extent() noexcept;
    [[nodiscard]] std::shared_ptr<const JobMemExtent> extent() const noexcept;

    [[nodiscard]] JobMmap::Ptr mmap() noexcept;
    [[nodiscard]] std::shared_ptr<const JobMmap> mmap() const noexcept;

private:
    [[nodiscard]] static bool alignUp(std::uintptr_t value, std::size_t alignment, std::uintptr_t &result) noexcept;
    [[nodiscard]] bool ownsLocked(const void *ptr) const noexcept;

    [[nodiscard]] Metrics metricsLocked() const noexcept;
    [[nodiscard]] ArenaMetrics arenaMetricsLocked() const noexcept;

    void init(std::size_t size);
    void init(JobMmap::Ptr mmap);
    void init(JobMemExtent::Ptr extent);

    void moveFrom(JobArenaPool &&other) noexcept;

    mutable std::mutex m_mutex;

    JobMemExtent::Ptr m_extent;

    std::size_t m_offset{0};
    std::size_t m_paddingBytes{0};
    std::size_t m_highWatermark{0};
    std::size_t m_allocationCount{0};
};

} // namespace job::io