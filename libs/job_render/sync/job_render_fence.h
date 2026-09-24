#pragma once

#include <chrono>
#include <memory>

namespace job::render {

class JobRenderFence
{
public:
    using Ptr  = std::shared_ptr<JobRenderFence>;
    using WPtr = std::weak_ptr<JobRenderFence>;
    using UPtr = std::unique_ptr<JobRenderFence>;

    using NativeHandle  = int;
    using Timeout       = std::chrono::milliseconds;

    JobRenderFence() = default;

    explicit JobRenderFence(NativeHandle fd) noexcept :
        m_fd(fd)
    {
    }

    ~JobRenderFence();

    JobRenderFence(const JobRenderFence &) = delete;
    JobRenderFence &operator=(const JobRenderFence &) = delete;

    JobRenderFence(JobRenderFence &&other) noexcept;
    JobRenderFence &operator=(JobRenderFence &&other) noexcept;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobRenderFence>();
    }

    [[nodiscard]] static Ptr createShared(NativeHandle fd)
    {
        return std::make_shared<JobRenderFence>(fd);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobRenderFence>();
    }

    [[nodiscard]] static UPtr createUniq(NativeHandle fd)
    {
        return std::make_unique<JobRenderFence>(fd);
    }

    void reset(NativeHandle fd = -1) noexcept;

    [[nodiscard]] NativeHandle release() noexcept;

    [[nodiscard]] NativeHandle fd() const noexcept
    {
        return m_fd;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_fd >= 0;
    }

    [[nodiscard]] bool signaled() const noexcept;

    [[nodiscard]] bool wait() const noexcept;

    [[nodiscard]] bool wait(Timeout timeout) const noexcept;
    [[nodiscard]] JobRenderFence duplicate() const noexcept;

private:
    NativeHandle m_fd{-1};
};

}