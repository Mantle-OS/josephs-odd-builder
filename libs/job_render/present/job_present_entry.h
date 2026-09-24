#pragma once

#include <memory>
#include <utility>

#include "job_drm_framebuffer_registration.h"
#include "job_gbm_buffer_lease.h"
#include "job_present_frame.h"

namespace job::render {

class JobPresentEntry
{
public:
    using Ptr  = std::shared_ptr<JobPresentEntry>;
    using WPtr = std::weak_ptr<JobPresentEntry>;
    using UPtr = std::unique_ptr<JobPresentEntry>;

    JobPresentEntry() = default;

    JobPresentEntry(JobPresentFrame frame, JobDrmFramebufferRegistration framebuffer, JobGbmBufferLease lease) noexcept :
        m_frame(std::move(frame)),
        m_lease(std::move(lease)),
        m_framebuffer(std::move(framebuffer))
    {
    }

    ~JobPresentEntry() = default;

    JobPresentEntry(const JobPresentEntry &) = delete;
    JobPresentEntry &operator=(const JobPresentEntry &) = delete;

    JobPresentEntry(JobPresentEntry &&) noexcept = default;

    JobPresentEntry &operator=(JobPresentEntry &&other) noexcept
    {
        if (this == &other)
            return *this;

        m_framebuffer = std::move(other.m_framebuffer);
        m_lease = std::move(other.m_lease);
        m_frame = std::move(other.m_frame);

        return *this;
    }

    [[nodiscard]] static Ptr createShared(JobPresentFrame frame, JobDrmFramebufferRegistration framebuffer, JobGbmBufferLease lease)
    {
        return std::make_shared<JobPresentEntry>(std::move(frame), std::move(framebuffer), std::move(lease));
    }

    [[nodiscard]] static UPtr createUniq(JobPresentFrame frame, JobDrmFramebufferRegistration framebuffer, JobGbmBufferLease lease)
    {
        return std::make_unique<JobPresentEntry>(std::move(frame), std::move(framebuffer), std::move(lease));
    }

    [[nodiscard]] JobPresentFrame &frame() noexcept
    {
        return m_frame;
    }

    [[nodiscard]] const JobPresentFrame &frame() const noexcept
    {
        return m_frame;
    }

    [[nodiscard]] JobGbmBufferLease &lease() noexcept
    {
        return m_lease;
    }

    [[nodiscard]] const JobGbmBufferLease &lease() const noexcept
    {
        return m_lease;
    }

    [[nodiscard]] JobDrmFramebufferRegistration &framebuffer() noexcept
    {
        return m_framebuffer;
    }

    [[nodiscard]] const JobDrmFramebufferRegistration &framebuffer() const noexcept
    {
        return m_framebuffer;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_frame.valid() && m_lease.valid() && m_framebuffer.valid() && m_frame.framebufferId() == m_framebuffer.id();
    }

private:
    //
    // Destruction order is intentional: registration must be removed before
    // the backing GBM buffer is returned to the surface. Move assignment is
    // therefore implemented explicitly above for the same ordering.
    //
    JobPresentFrame m_frame;
    JobGbmBufferLease m_lease;
    JobDrmFramebufferRegistration m_framebuffer;
};

}