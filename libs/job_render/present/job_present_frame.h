#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "job_drm_damage.h"
#include "job_drm_framebuffer.h"
#include "job_render_fence.h"

namespace job::render {

class JobPresentFrame
{
public:
    using Ptr  = std::shared_ptr<JobPresentFrame>;
    using WPtr = std::weak_ptr<JobPresentFrame>;
    using UPtr = std::unique_ptr<JobPresentFrame>;

    using Id                = std::uint64_t;
    using FramebufferId     = JobDrmFramebuffer::Id;

    JobPresentFrame() = default;

    explicit JobPresentFrame(Id id, FramebufferId framebufferId) noexcept :
        m_id(id),
        m_framebufferId(framebufferId)
    {
    }

    ~JobPresentFrame() = default;

    JobPresentFrame(const JobPresentFrame &) = delete;
    JobPresentFrame &operator=(const JobPresentFrame &) = delete;

    JobPresentFrame(JobPresentFrame &&) noexcept = default;
    JobPresentFrame &operator=(JobPresentFrame &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobPresentFrame>();
    }

    [[nodiscard]] static Ptr createShared(Id id, FramebufferId framebufferId)
    {
        return std::make_shared<JobPresentFrame>(id, framebufferId);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobPresentFrame>();
    }

    [[nodiscard]] static UPtr createUniq(Id id, FramebufferId framebufferId)
    {
        return std::make_unique<JobPresentFrame>(id, framebufferId);
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    void setId(Id id) noexcept
    {
        m_id = id;
    }

    [[nodiscard]] constexpr FramebufferId framebufferId() const noexcept
    {
        return m_framebufferId;
    }

    void setFramebufferId(FramebufferId framebufferId) noexcept
    {
        m_framebufferId = framebufferId;
    }

    [[nodiscard]] const std::optional<JobDrmDamage> &damage() const noexcept
    {
        return m_damage;
    }

    void setDamage(JobDrmDamage damage)
    {
        m_damage = std::move(damage);
    }

    void clearDamage() noexcept
    {
        m_damage.reset();
    }

    [[nodiscard]] bool hasDamage() const noexcept
    {
        return m_damage.has_value();
    }

    [[nodiscard]] const JobRenderFence &acquireFence() const noexcept
    {
        return m_acquireFence;
    }

    [[nodiscard]] JobRenderFence &acquireFence() noexcept
    {
        return m_acquireFence;
    }

    void setAcquireFence(JobRenderFence fence) noexcept
    {
        m_acquireFence = std::move(fence);
    }

    [[nodiscard]] JobRenderFence releaseAcquireFence() noexcept
    {
        return std::move(m_acquireFence);
    }

    [[nodiscard]] const JobRenderFence &releaseFence() const noexcept
    {
        return m_releaseFence;
    }

    [[nodiscard]] JobRenderFence &releaseFence() noexcept
    {
        return m_releaseFence;
    }

    void setReleaseFence(JobRenderFence fence) noexcept
    {
        m_releaseFence = std::move(fence);
    }

    [[nodiscard]] JobRenderFence releaseReleaseFence() noexcept
    {
        return std::move(m_releaseFence);
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_framebufferId != 0;
    }

private:
    Id                              m_id{};
    FramebufferId                   m_framebufferId{};
    std::optional<JobDrmDamage>     m_damage;
    JobRenderFence                  m_acquireFence;
    JobRenderFence                  m_releaseFence;
};

}