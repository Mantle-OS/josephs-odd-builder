#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "job_mem_pool.h"
#include "job_mem_size.h"
#include "job_page_pool.h"

namespace job::io {

class JobSizePool final : public JobMemPool
{
public:
    using Ptr  = std::shared_ptr<JobSizePool>;
    using WPtr = std::weak_ptr<JobSizePool>;
    using UPtr = std::unique_ptr<JobSizePool>;

    struct SizeMetrics final
    {
        std::size_t objectSize{0};
        std::size_t alignment{0};

        std::size_t objectCapacity{0};
        std::size_t allocatedObjects{0};
        std::size_t availableObjects{0};

        std::size_t spanCount{0};
        std::size_t pagesPerSpan{0};

        std::size_t reservedBytes{0};
        std::size_t internalWasteBytes{0};
    };

    explicit JobSizePool(JobMemSize sizeClass, JobPagePool::Ptr pagePool);

    ~JobSizePool() override;

    JobSizePool(const JobSizePool &) = delete;
    JobSizePool &operator=(const JobSizePool &) = delete;

    JobSizePool(JobSizePool &&other) noexcept;
    JobSizePool &operator=(JobSizePool &&other) noexcept;

    [[nodiscard]] static Ptr createShared(JobMemSize sizeClass, JobPagePool::Ptr pagePool);

    [[nodiscard]] static UPtr createUniq(JobMemSize sizeClass, JobPagePool::Ptr pagePool);

    [[nodiscard]] Type type() const noexcept override final;

    [[nodiscard]] void *alloc(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) override final;
    [[nodiscard]] void *allocObject();
    [[nodiscard]] bool free(void *ptr) override final;
    [[nodiscard]] bool owns(const void *ptr) const noexcept override final;

    [[nodiscard]] std::size_t size() const noexcept override final;
    [[nodiscard]] std::size_t allocated() const noexcept override final;
    [[nodiscard]] std::size_t available() const noexcept override final;

    [[nodiscard]] Metrics metrics() const noexcept override final;
    [[nodiscard]] SizeMetrics sizeMetrics() const noexcept;

    void clear() override final;

    [[nodiscard]] const JobMemSize &sizeClass() const noexcept;

    [[nodiscard]] std::size_t objectSize() const noexcept;
    [[nodiscard]] std::size_t alignment() const noexcept;

    [[nodiscard]] std::size_t objectCapacity() const noexcept;
    [[nodiscard]] std::size_t allocatedObjects() const noexcept;
    [[nodiscard]] std::size_t availableObjects() const noexcept;

    [[nodiscard]] JobPagePool::Ptr pagePool() noexcept;
    [[nodiscard]] std::shared_ptr<const JobPagePool> pagePool() const noexcept;

private:
    struct PageAllocation final
    {
        void *addr{nullptr};
        std::size_t pageCount{0};
        std::size_t objectCount{0};
    };

    struct Allocation final
    {
        std::size_t requestedSize{0};
    };

    [[nodiscard]] bool grow();

    void buildFreeList(void *addr, std::size_t objectCount);
    [[nodiscard]] bool validRequest(std::size_t size, std::size_t alignment) const noexcept;

    [[nodiscard]] bool ownsLocked(const void *ptr) const noexcept;
    [[nodiscard]] bool isObjectBoundaryLocked(const void *ptr) const noexcept;

    [[nodiscard]] Metrics metricsLocked() const noexcept;
    [[nodiscard]] SizeMetrics sizeMetricsLocked() const noexcept;

    void init(JobPagePool::Ptr pagePool);
    void moveFrom(JobSizePool &&other) noexcept;

    mutable std::mutex                      m_mutex;
    JobMemSize                              m_sizeClass;
    JobPagePool::Ptr                        m_pagePool;
    std::vector<PageAllocation>             m_pageAllocations;
    std::vector<void*>                      m_freeObjects;
    std::unordered_map<void *, Allocation>  m_allocations;
    std::size_t                             m_objectCapacity{0};
    std::size_t                             m_allocatedObjects{0};
};

} // namespace job::io