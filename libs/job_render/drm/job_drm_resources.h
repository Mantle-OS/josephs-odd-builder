#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <xf86drmMode.h>

namespace job::render {

class JobDrmResources
{
public:
    using Ptr  = std::shared_ptr<JobDrmResources>;
    using WPtr = std::weak_ptr<JobDrmResources>;
    using UPtr = std::unique_ptr<JobDrmResources>;

    using Id = std::uint32_t;

    using FramebufferIds = std::vector<Id>;
    using CrtcIds        = std::vector<Id>;
    using ConnectorIds   = std::vector<Id>;
    using EncoderIds     = std::vector<Id>;

    JobDrmResources() = default;
    ~JobDrmResources() = default;

    JobDrmResources(const JobDrmResources &) = default;
    JobDrmResources &operator=(const JobDrmResources &) = default;

    JobDrmResources(JobDrmResources &&) noexcept = default;
    JobDrmResources &operator=(JobDrmResources &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmResources>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmResources>();
    }

    [[nodiscard]] static JobDrmResources fromDrm(const drmModeRes &resources);

    [[nodiscard]] const FramebufferIds &framebufferIds() const noexcept
    {
        return m_framebufferIds;
    }

    [[nodiscard]] const CrtcIds &crtcIds() const noexcept
    {
        return m_crtcIds;
    }

    [[nodiscard]] const ConnectorIds &connectorIds() const noexcept
    {
        return m_connectorIds;
    }

    [[nodiscard]] const EncoderIds &encoderIds() const noexcept
    {
        return m_encoderIds;
    }

    [[nodiscard]] std::uint32_t minWidth() const noexcept
    {
        return m_minWidth;
    }

    [[nodiscard]] std::uint32_t maxWidth() const noexcept
    {
        return m_maxWidth;
    }

    [[nodiscard]] std::uint32_t minHeight() const noexcept
    {
        return m_minHeight;
    }

    [[nodiscard]] std::uint32_t maxHeight() const noexcept
    {
        return m_maxHeight;
    }

    [[nodiscard]] std::size_t framebufferCount() const noexcept
    {
        return m_framebufferIds.size();
    }

    [[nodiscard]] std::size_t crtcCount() const noexcept
    {
        return m_crtcIds.size();
    }

    [[nodiscard]] std::size_t connectorCount() const noexcept
    {
        return m_connectorIds.size();
    }

    [[nodiscard]] std::size_t encoderCount() const noexcept
    {
        return m_encoderIds.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_framebufferIds.empty() &&
               m_crtcIds.empty() &&
               m_connectorIds.empty() &&
               m_encoderIds.empty();
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return !m_crtcIds.empty() &&
               !m_connectorIds.empty() &&
               m_minWidth <= m_maxWidth &&
               m_minHeight <= m_maxHeight;
    }

private:
    FramebufferIds  m_framebufferIds;
    CrtcIds         m_crtcIds;
    ConnectorIds    m_connectorIds;
    EncoderIds      m_encoderIds;

    std::uint32_t   m_minWidth{};
    std::uint32_t   m_maxWidth{};
    std::uint32_t   m_minHeight{};
    std::uint32_t   m_maxHeight{};
};

}