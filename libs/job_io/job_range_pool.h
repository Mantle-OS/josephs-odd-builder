#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "job_mem_pool.h"
#include "job_mem_range.h"
#include "job_mmap.h"

namespace job::io {

class JobRangePool final : public JobMemPool
{
public:
    using Ptr  = std::shared_ptr<JobRangePool>;
    using WPtr = std::weak_ptr<JobRangePool>;
    using UPtr = std::unique_ptr<JobRangePool>;

    using Ranges = std::vector<JobMemRange>;

    struct RangeMetrics final
    {
        std::size_t largestFreeBlock{0};
        std::size_t freeRangeCount{0};
    };

    explicit JobRangePool(std::size_t size);
    explicit JobRangePool(JobMmap::Ptr mmap);

    ~JobRangePool() override;

    JobRangePool(const JobRangePool &) = delete;
    JobRangePool &operator=(const JobRangePool &) = delete;

    JobRangePool(JobRangePool &&other) noexcept;
    JobRangePool &operator=(JobRangePool &&other) noexcept;

    [[nodiscard]] static Ptr createShared(std::size_t size);
    [[nodiscard]] static Ptr createShared(JobMmap::Ptr mmap);

    [[nodiscard]] static UPtr createUniq(std::size_t size);
    [[nodiscard]] static UPtr createUniq(JobMmap::Ptr mmap);

    [[nodiscard]] Type type() const noexcept override final;

    [[nodiscard]] void *alloc(
        std::size_t size,
        std::size_t alignment = alignof(std::max_align_t)) override final;

    [[nodiscard]] bool free(void *ptr) override final;

    [[nodiscard]] bool owns(const void *ptr) const noexcept override final;

    [[nodiscard]] std::size_t size() const noexcept override final;
    [[nodiscard]] std::size_t allocated() const noexcept override final;
    [[nodiscard]] std::size_t available() const noexcept override final;

    [[nodiscard]] Metrics metrics() const noexcept override final;
    [[nodiscard]] RangeMetrics rangeMetrics() const noexcept;

    void clear() override final;

    [[nodiscard]] JobMmap::Ptr mmap() noexcept;
    [[nodiscard]] std::shared_ptr<const JobMmap> mmap() const noexcept;

private:
    [[nodiscard]] static std::size_t alignUp(
        std::size_t value,
        std::size_t alignment) noexcept;

    [[nodiscard]] bool findFreeRange(
        std::size_t size,
        std::size_t alignment,
        std::size_t &rangeIndex,
        std::size_t &offset) const noexcept;

    void splitRange(
        std::size_t rangeIndex,
        std::size_t offset,
        std::size_t size);

    void insertFreeRange(JobMemRange range);
    void coalesceRanges();

    [[nodiscard]] std::size_t offsetOf(const void *ptr) const noexcept;

    [[nodiscard]] void *ptrAt(std::size_t offset) noexcept;
    [[nodiscard]] const void *ptrAt(std::size_t offset) const noexcept;

    [[nodiscard]] Metrics metricsLocked() const noexcept;
    [[nodiscard]] RangeMetrics rangeMetricsLocked() const noexcept;

    void init(std::size_t size);
    void init(JobMmap::Ptr mmap);

    void moveFrom(JobRangePool &&other) noexcept;

    [[nodiscard]] bool ownsLocked(const void *ptr) const noexcept;

    mutable std::mutex m_mutex;

    JobMmap::Ptr m_mmap;

    Ranges m_freeRanges;

    std::unordered_map<std::size_t, std::size_t> m_allocations;

    std::size_t m_allocatedBytes{0};
};

} // namespace job::io