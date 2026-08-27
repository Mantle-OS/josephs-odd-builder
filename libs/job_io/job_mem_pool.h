#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <job_thread_pool.h>
#include <job_thread_options.h>

namespace job::io {

class JobMemPool
{
public:
    using Ptr  = std::shared_ptr<JobMemPool>;
    using WPtr = std::weak_ptr<JobMemPool>;
    using UPtr = std::unique_ptr<JobMemPool>;

    enum class Type : uint8_t
    {
        Unknown = 0,
        Range,
        Page,
        Size,
        Arena
    };

    struct Metrics final
    {
        std::size_t capacityBytes{0};
        std::size_t allocatedBytes{0};
        std::size_t freeBytes{0};
        std::size_t allocationCount{0};
    };

    JobMemPool() = default;
    virtual ~JobMemPool() = default;

    JobMemPool(const JobMemPool &) = delete;
    JobMemPool &operator=(const JobMemPool &) = delete;
    JobMemPool(JobMemPool &&) noexcept = default;
    JobMemPool &operator=(JobMemPool &&) noexcept = default;

    [[nodiscard]] virtual Type type() const noexcept = 0;

    [[nodiscard]] virtual void *alloc(std::size_t size,
                                      std::size_t alignment = alignof(std::max_align_t)) = 0;

    [[nodiscard]] virtual bool free(void *ptr) = 0;

    [[nodiscard]] virtual bool owns(const void *ptr) const noexcept = 0;

    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
    [[nodiscard]] virtual std::size_t allocated() const noexcept = 0;
    [[nodiscard]] virtual std::size_t available() const noexcept = 0;

    [[nodiscard]] virtual Metrics metrics() const noexcept = 0;

    virtual void clear() = 0;

};

} // namespace job::io