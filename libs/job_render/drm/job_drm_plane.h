#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <xf86drmMode.h>

#include "job_drm_format.h"
#include "job_drm_format_modifier.h"
#include "job_drm_object_properties.h"

namespace job::render {

class JobDrmPlane
{
public:
    using Ptr  = std::shared_ptr<JobDrmPlane>;
    using WPtr = std::weak_ptr<JobDrmPlane>;
    using UPtr = std::unique_ptr<JobDrmPlane>;

    using Id              = std::uint32_t;
    using CrtcId          = std::uint32_t;
    using FramebufferId   = std::uint32_t;
    using CrtcMask        = std::uint32_t;
    using Formats         = std::vector<JobDrmFormat>;
    using FormatModifiers = std::vector<JobDrmFormatModifier>;

    JobDrmPlane() = default;

    JobDrmPlane(Id id,
                CrtcId crtcId,
                FramebufferId framebufferId,
                std::uint32_t crtcX,
                std::uint32_t crtcY,
                std::uint32_t x,
                std::uint32_t y,
                CrtcMask possibleCrtcs,
                Formats formats,
                FormatModifiers formatModifiers,
                bool explicitModifiers,
                JobDrmObjectProperties properties) :
        m_id(id),
        m_crtcId(crtcId),
        m_framebufferId(framebufferId),
        m_crtcX(crtcX),
        m_crtcY(crtcY),
        m_x(x),
        m_y(y),
        m_possibleCrtcs(possibleCrtcs),
        m_formats(std::move(formats)),
        m_formatModifiers(std::move(formatModifiers)),
        m_explicitModifiers(explicitModifiers),
        m_properties(std::move(properties))
    {
    }

    ~JobDrmPlane() = default;

    JobDrmPlane(const JobDrmPlane &) = default;
    JobDrmPlane &operator=(const JobDrmPlane &) = default;

    JobDrmPlane(JobDrmPlane &&) noexcept = default;
    JobDrmPlane &operator=(JobDrmPlane &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          CrtcId crtcId,
                                          FramebufferId framebufferId,
                                          std::uint32_t crtcX,
                                          std::uint32_t crtcY,
                                          std::uint32_t x,
                                          std::uint32_t y,
                                          CrtcMask possibleCrtcs,
                                          Formats formats,
                                          FormatModifiers formatModifiers,
                                          bool explicitModifiers,
                                          JobDrmObjectProperties properties)
    {
        return std::make_shared<JobDrmPlane>(
            id,
            crtcId,
            framebufferId,
            crtcX,
            crtcY,
            x,
            y,
            possibleCrtcs,
            std::move(formats),
            std::move(formatModifiers),
            explicitModifiers,
            std::move(properties));
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         CrtcId crtcId,
                                         FramebufferId framebufferId,
                                         std::uint32_t crtcX,
                                         std::uint32_t crtcY,
                                         std::uint32_t x,
                                         std::uint32_t y,
                                         CrtcMask possibleCrtcs,
                                         Formats formats,
                                         FormatModifiers formatModifiers,
                                         bool explicitModifiers,
                                         JobDrmObjectProperties properties)
    {
        return std::make_unique<JobDrmPlane>(
            id,
            crtcId,
            framebufferId,
            crtcX,
            crtcY,
            x,
            y,
            possibleCrtcs,
            std::move(formats),
            std::move(formatModifiers),
            explicitModifiers,
            std::move(properties));
    }

    [[nodiscard]] static JobDrmPlane fromDrm(const drmModePlane &plane,
                                             FormatModifiers formatModifiers,
                                             bool explicitModifiers,
                                             JobDrmObjectProperties properties)
    {
        Formats formats;
        formats.reserve(plane.count_formats);

        for (std::uint32_t i = 0; i < plane.count_formats; ++i)
            formats.emplace_back(plane.formats[i]);

        return JobDrmPlane{
            plane.plane_id,
            plane.crtc_id,
            plane.fb_id,
            plane.crtc_x,
            plane.crtc_y,
            plane.x,
            plane.y,
            plane.possible_crtcs,
            std::move(formats),
            std::move(formatModifiers),
            explicitModifiers,
            std::move(properties)
        };
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr CrtcId crtcId() const noexcept
    {
        return m_crtcId;
    }

    [[nodiscard]] constexpr FramebufferId framebufferId() const noexcept
    {
        return m_framebufferId;
    }

    [[nodiscard]] constexpr std::uint32_t crtcX() const noexcept
    {
        return m_crtcX;
    }

    [[nodiscard]] constexpr std::uint32_t crtcY() const noexcept
    {
        return m_crtcY;
    }

    [[nodiscard]] constexpr std::uint32_t x() const noexcept
    {
        return m_x;
    }

    [[nodiscard]] constexpr std::uint32_t y() const noexcept
    {
        return m_y;
    }

    [[nodiscard]] constexpr CrtcMask possibleCrtcs() const noexcept
    {
        return m_possibleCrtcs;
    }

    [[nodiscard]] constexpr bool supportsCrtc(std::uint32_t crtcIndex) const noexcept
    {
        if (crtcIndex >= 32)
            return false;

        return (m_possibleCrtcs & (std::uint32_t{1} << crtcIndex)) != 0;
    }

    [[nodiscard]] std::span<const JobDrmFormat> formats() const noexcept
    {
        return m_formats;
    }

    [[nodiscard]] std::span<const JobDrmFormatModifier> formatModifiers() const noexcept
    {
        return m_formatModifiers;
    }

    [[nodiscard]] constexpr bool hasExplicitModifiers() const noexcept
    {
        return m_explicitModifiers;
    }

    [[nodiscard]] bool supportsFormat(JobDrmFormat format) const noexcept
    {
        for (const auto candidate : m_formats) {
            if (candidate == format)
                return true;
        }

        return false;
    }

    [[nodiscard]] bool supportsFormatModifier(JobDrmFormat format, JobDrmModifier modifier) const noexcept
    {
        for (const auto &candidate : m_formatModifiers) {
            if (candidate.format() == format && candidate.modifier() == modifier)
                return true;
        }

        return false;
    }

    [[nodiscard]] const JobDrmObjectProperties &properties() const noexcept
    {
        return m_properties;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    [[nodiscard]] friend bool operator==(const JobDrmPlane &, const JobDrmPlane &) = default;

private:
    Id                     m_id{};
    CrtcId                 m_crtcId{};
    FramebufferId          m_framebufferId{};
    std::uint32_t          m_crtcX{};
    std::uint32_t          m_crtcY{};
    std::uint32_t          m_x{};
    std::uint32_t          m_y{};
    CrtcMask               m_possibleCrtcs{};
    Formats                m_formats;
    FormatModifiers        m_formatModifiers;
    bool                   m_explicitModifiers{};
    JobDrmObjectProperties m_properties;
};

} // namespace job::render