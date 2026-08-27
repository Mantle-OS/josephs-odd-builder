#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "job_mem_range.h"

namespace job::io {

class JobMemLock final
{
public:
    using Ptr  = std::shared_ptr<JobMemLock>;
    using WPtr = std::weak_ptr<JobMemLock>;
    using UPtr = std::unique_ptr<JobMemLock>;

    using Ranges = std::vector<JobMemRange>;

    JobMemLock() = default;

    JobMemLock(void *addr, std::size_t size, std::size_t pageSize)
        pre(addr != nullptr)
        pre(size > 0)
        pre(pageSize > 0)
        pre(JobMemRange::validAlignment(pageSize));

    ~JobMemLock();

    JobMemLock(const JobMemLock &) = delete;
    JobMemLock &operator=(const JobMemLock &) = delete;

    JobMemLock(JobMemLock &&other) noexcept;
    JobMemLock &operator=(JobMemLock &&other) noexcept;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobMemLock>();
    }

    [[nodiscard]] static Ptr createShared(void *addr, std::size_t size, std::size_t pageSize)
        pre(addr != nullptr)
        pre(size > 0)
        pre(pageSize > 0)
        pre(JobMemRange::validAlignment(pageSize))
    {
        return std::make_shared<JobMemLock>(addr, size, pageSize);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobMemLock>();
    }

    [[nodiscard]] static UPtr createUniq(void *addr, std::size_t size, std::size_t pageSize)
        pre(addr != nullptr)
        pre(size > 0)
        pre(pageSize > 0)
        pre(JobMemRange::validAlignment(pageSize))
    {
        return std::make_unique<JobMemLock>(addr, size, pageSize);
    }

    [[nodiscard]] bool reset(void *addr, std::size_t size, std::size_t pageSize)
        pre(addr != nullptr)
        pre(size > 0)
        pre(pageSize > 0)
        pre(JobMemRange::validAlignment(pageSize));

    [[nodiscard]] bool clear() noexcept;

    [[nodiscard]] void *addr() noexcept;
    [[nodiscard]] const void *addr() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t pageSize() const noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool release() noexcept;

    [[nodiscard]] bool lock();
    [[nodiscard]] bool lock(const JobMemRange &range);

    [[nodiscard]] bool unlock();
    [[nodiscard]] bool unlock(const JobMemRange &range);

    [[nodiscard]] bool isLocked() const noexcept;
    [[nodiscard]] bool isLocked(std::size_t offset) const noexcept;
    [[nodiscard]] bool isLocked(const JobMemRange &range) const noexcept;

    [[nodiscard]] std::size_t lockedSize() const noexcept;
    [[nodiscard]] const Ranges &lockedRanges() const noexcept;

private:
    [[nodiscard]] JobMemRange alignedRange(const JobMemRange &range) const
        pre(valid())
        pre(!range.empty())
        pre(range.last() <= m_size);

    [[nodiscard]] bool addLockedRange(const JobMemRange &range);
    void removeLockedRange(const JobMemRange &range);

    void moveFrom(JobMemLock &&other) noexcept;

    void *m_addr{nullptr};

    std::size_t m_size{0};
    std::size_t m_pageSize{0};

    Ranges m_lockedRanges;
};

} // namespace job::io