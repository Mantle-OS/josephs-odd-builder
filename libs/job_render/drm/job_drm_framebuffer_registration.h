#pragma once

#include <cstdint>
#include <memory>
#include <utility>

namespace job::render {

class JobDrmFramebufferRegistration
{
public:
    using Ptr  = std::shared_ptr<JobDrmFramebufferRegistration>;
    using WPtr = std::weak_ptr<JobDrmFramebufferRegistration>;
    using UPtr = std::unique_ptr<JobDrmFramebufferRegistration>;

    using Id            = std::uint32_t;
    using NativeHandle  = int;

    JobDrmFramebufferRegistration() = default;

    JobDrmFramebufferRegistration(NativeHandle  drmFd,Id framebufferId) noexcept :
        m_drmFd(drmFd),
        m_framebufferId(framebufferId)
    {
    }

    ~JobDrmFramebufferRegistration();

    JobDrmFramebufferRegistration(const JobDrmFramebufferRegistration &) = delete;
    JobDrmFramebufferRegistration &operator=(const JobDrmFramebufferRegistration &) = delete;
    JobDrmFramebufferRegistration(JobDrmFramebufferRegistration &&other) noexcept;
    JobDrmFramebufferRegistration &operator=(JobDrmFramebufferRegistration &&other) noexcept;

    [[nodiscard]] static Ptr createShared(NativeHandle drmFd, Id framebufferId)
    {
        return std::make_shared<JobDrmFramebufferRegistration>(drmFd, framebufferId);
    }

    [[nodiscard]] static UPtr createUniq(NativeHandle drmFd, Id framebufferId)
    {
        return std::make_unique<JobDrmFramebufferRegistration>(drmFd, framebufferId);
    }

    void reset() noexcept;

    void reset(NativeHandle drmFd, Id framebufferId) noexcept;
    [[nodiscard]] Id release() noexcept;

    [[nodiscard]] NativeHandle drmFd() const noexcept
    {
        return m_drmFd;
    }

    [[nodiscard]] Id id() const noexcept
    {
        return m_framebufferId;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_drmFd >= 0 && m_framebufferId != 0;
    }

private:
    NativeHandle    m_drmFd{-1}; // BORROWED
    Id              m_framebufferId{};
};

}