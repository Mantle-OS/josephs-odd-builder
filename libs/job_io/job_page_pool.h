#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "job_mem_extent.h"
#include "job_mem_page.h"
#include "job_mem_pool.h"
#include "job_mem_span.h"
#include "job_mmap.h"

namespace job::io {

class JobPagePool final : public JobMemPool
{
public:
    using Ptr  = std::shared_ptr<JobPagePool>;
    using WPtr = std::weak_ptr<JobPagePool>;
    using UPtr = std::unique_ptr<JobPagePool>;

    using Pages = std::vector<JobMemPage>;
    using Spans = std::vector<JobMemSpan>;

    struct PageMetrics final
    {
        std::size_t pageSize{0};
        std::size_t pageCount{0};
        std::size_t allocatedPages{0};
        std::size_t availablePages{0};
        std::size_t freeSpanCount{0};
        std::size_t largestFreeSpanPages{0};
    };

    explicit JobPagePool(
        std::size_t size,
        std::size_t pageSize = systemPageSize());

    explicit JobPagePool(
        JobMmap::Ptr mmap,
        std::size_t pageSize = systemPageSize());

    explicit JobPagePool(
        JobMemExtent::Ptr extent,
        std::size_t pageSize = systemPageSize());

    ~JobPagePool() override;

    JobPagePool(const JobPagePool &) = delete;
    JobPagePool &operator=(const JobPagePool &) = delete;

    JobPagePool(JobPagePool &&other) noexcept;
    JobPagePool &operator=(JobPagePool &&other) noexcept;

    [[nodiscard]] static Ptr createShared(
        std::size_t size,
        std::size_t pageSize = systemPageSize());

    [[nodiscard]] static Ptr createShared(
        JobMmap::Ptr mmap,
        std::size_t pageSize = systemPageSize());

    [[nodiscard]] static Ptr createShared(
        JobMemExtent::Ptr extent,
        std::size_t pageSize = systemPageSize());

    [[nodiscard]] static UPtr createUniq(
        std::size_t size,
        std::size_t pageSize = systemPageSize());

    [[nodiscard]] static UPtr createUniq(
        JobMmap::Ptr mmap,
        std::size_t pageSize = systemPageSize());

    [[nodiscard]] static UPtr createUniq(
        JobMemExtent::Ptr extent,
        std::size_t pageSize = systemPageSize());

    [[nodiscard]] Type type() const noexcept override final;

    [[nodiscard]] void *alloc(
        std::size_t size,
        std::size_t alignment = alignof(std::max_align_t)) override final;

    [[nodiscard]] void *allocPages(std::size_t pageCount);

    [[nodiscard]] bool free(void *ptr) override final;

    [[nodiscard]] bool owns(const void *ptr) const noexcept override final;

    [[nodiscard]] std::size_t size() const noexcept override final;
    [[nodiscard]] std::size_t allocated() const noexcept override final;
    [[nodiscard]] std::size_t available() const noexcept override final;

    [[nodiscard]] Metrics metrics() const noexcept override final;
    [[nodiscard]] PageMetrics pageMetrics() const noexcept;

    void clear() override final;

    [[nodiscard]] std::size_t pageSize() const noexcept;
    [[nodiscard]] std::size_t pageCount() const noexcept;
    [[nodiscard]] std::size_t allocatedPages() const noexcept;
    [[nodiscard]] std::size_t availablePages() const noexcept;

    [[nodiscard]] const Pages &pages() const noexcept;

    [[nodiscard]] JobMemExtent::Ptr extent() noexcept;
    [[nodiscard]] std::shared_ptr<const JobMemExtent> extent() const noexcept;

    [[nodiscard]] JobMmap::Ptr mmap() noexcept;
    [[nodiscard]] std::shared_ptr<const JobMmap> mmap() const noexcept;

private:
    struct Allocation final
    {
        JobMemPage::Index firstPageIndex{JobMemPage::kInvalidIndex};
        std::size_t pageCount{0};
        std::size_t requestedSize{0};
    };

    [[nodiscard]] static std::size_t pagesForSize(
        std::size_t size,
        std::size_t pageSize) noexcept;

    [[nodiscard]] bool validPageGeometry(
        const JobMemExtent &extent,
        std::size_t pageSize) const noexcept;

    [[nodiscard]] bool findFreeSpan(
        std::size_t pageCount,
        std::size_t alignment,
        std::size_t &spanIndex,
        JobMemPage::Index &firstPageIndex) const noexcept;

    void splitSpan(
        std::size_t spanIndex,
        JobMemPage::Index firstPageIndex,
        std::size_t pageCount);

    void insertFreeSpan(JobMemSpan span);
    void coalesceSpans();

    [[nodiscard]] JobMemSpan makeSpan(
        JobMemPage::Index firstPageIndex,
        std::size_t pageCount);

    [[nodiscard]] bool ownsLocked(const void *ptr) const noexcept;

    [[nodiscard]] std::size_t offsetOf(const void *ptr) const noexcept;
    [[nodiscard]] JobMemPage::Index pageIndexOf(const void *ptr) const noexcept;

    [[nodiscard]] void *ptrAtPage(JobMemPage::Index pageIndex) noexcept;
    [[nodiscard]] const void *ptrAtPage(JobMemPage::Index pageIndex) const noexcept;

    [[nodiscard]] Metrics metricsLocked() const noexcept;
    [[nodiscard]] PageMetrics pageMetricsLocked() const noexcept;

    void init(
        std::size_t size,
        std::size_t pageSize);

    void init(
        JobMmap::Ptr mmap,
        std::size_t pageSize);

    void init(
        JobMemExtent::Ptr extent,
        std::size_t pageSize);

    void buildPages();
    void moveFrom(JobPagePool &&other) noexcept;

    mutable std::mutex m_mutex;

    JobMemExtent::Ptr m_extent;

    std::size_t m_pageSize{0};

    Pages m_pages;
    Spans m_freeSpans;

    std::unordered_map<JobMemPage::Index, Allocation> m_allocations;

    std::size_t m_allocatedPages{0};
    std::size_t m_allocatedBytes{0};

    JobMemSpan::Id m_nextSpanId{0};
};

} // namespace job::io