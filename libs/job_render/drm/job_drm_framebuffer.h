#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>

#include <xf86drmMode.h>

#include "job_drm_format.h"
#include "job_drm_modifier.h"

namespace job::render {

class JobDrmFramebuffer
{
public:
    using Ptr  = std::shared_ptr<JobDrmFramebuffer>;
    using WPtr = std::weak_ptr<JobDrmFramebuffer>;
    using UPtr = std::unique_ptr<JobDrmFramebuffer>;

    using Id     = std::uint32_t;
    using Handle = std::uint32_t;
    using Pitch  = std::uint32_t;
    using Offset = std::uint32_t;
    using Flags  = std::uint32_t;

    static constexpr std::size_t kMaxPlanes = 4;

    struct Plane
    {
        Handle handle{};
        Pitch pitch{};
        Offset offset{};

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return handle != 0 || pitch != 0;
        }

        [[nodiscard]] friend bool operator==(const Plane &, const Plane &) = default;
    };

    using Planes = std::array<Plane, kMaxPlanes>;

    JobDrmFramebuffer() = default;

    JobDrmFramebuffer(Id id,
                      std::uint32_t width,
                      std::uint32_t height,
                      JobDrmFormat format,
                      JobDrmModifier modifier,
                      Flags flags,
                      Planes planes,
                      std::size_t planeCount) :
        m_id(id),
        m_width(width),
        m_height(height),
        m_format(std::move(format)),
        m_modifier(std::move(modifier)),
        m_flags(flags),
        m_planes(std::move(planes)),
        m_planeCount(planeCount)
    {
    }

    ~JobDrmFramebuffer() = default;

    JobDrmFramebuffer(const JobDrmFramebuffer &) = default;
    JobDrmFramebuffer &operator=(const JobDrmFramebuffer &) = default;

    JobDrmFramebuffer(JobDrmFramebuffer &&) noexcept = default;
    JobDrmFramebuffer &operator=(JobDrmFramebuffer &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          std::uint32_t width,
                                          std::uint32_t height,
                                          JobDrmFormat format,
                                          JobDrmModifier modifier,
                                          Flags flags,
                                          Planes planes,
                                          std::size_t planeCount)
    {
        return std::make_shared<JobDrmFramebuffer>(id,
                                                   width,
                                                   height,
                                                   std::move(format),
                                                   std::move(modifier),
                                                   flags,
                                                   std::move(planes),
                                                   planeCount);
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         std::uint32_t width,
                                         std::uint32_t height,
                                         JobDrmFormat format,
                                         JobDrmModifier modifier,
                                         Flags flags,
                                         Planes planes,
                                         std::size_t planeCount)
    {
        return std::make_unique<JobDrmFramebuffer>(id,
                                                   width,
                                                   height,
                                                   std::move(format),
                                                   std::move(modifier),
                                                   flags,
                                                   std::move(planes),
                                                   planeCount);
    }

    [[nodiscard]] static JobDrmFramebuffer fromDrm(const drmModeFB2 &framebuffer)
    {
        Planes planes{};
        std::size_t planeCount = 0;

        const bool hasModifiers =
            (framebuffer.flags & DRM_MODE_FB_MODIFIERS) != 0;

        for (std::size_t i = 0; i < kMaxPlanes; ++i) {
            if (framebuffer.handles[i] == 0 &&
                framebuffer.pitches[i] == 0) {
                break;
            }

            planes[i] = Plane{
                framebuffer.handles[i],
                framebuffer.pitches[i],
                framebuffer.offsets[i]
            };

            planeCount = i + 1;
        }

        return JobDrmFramebuffer{
            framebuffer.fb_id,
            framebuffer.width,
            framebuffer.height,
            JobDrmFormat{framebuffer.pixel_format},
            hasModifiers
                ? JobDrmModifier{framebuffer.modifier}
                : JobDrmModifier::invalid(),
            framebuffer.flags,
            std::move(planes),
            planeCount
        };
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr std::uint32_t width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr std::uint32_t height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] const JobDrmFormat &format() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] const JobDrmModifier &modifier() const noexcept
    {
        return m_modifier;
    }

    [[nodiscard]] constexpr Flags flags() const noexcept
    {
        return m_flags;
    }

    [[nodiscard]] constexpr bool hasModifiers() const noexcept
    {
        return (m_flags & DRM_MODE_FB_MODIFIERS) != 0;
    }

    [[nodiscard]] constexpr std::size_t planeCount() const noexcept
    {
        return m_planeCount;
    }

    [[nodiscard]] std::span<const Plane> planes() const noexcept
    {
        return std::span<const Plane>{m_planes.data(), m_planeCount};
    }

    [[nodiscard]] const Plane *plane(std::size_t index) const noexcept
    {
        if (index >= m_planeCount)
            return nullptr;

        return &m_planes[index];
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    [[nodiscard]] friend bool operator==(const JobDrmFramebuffer &, const JobDrmFramebuffer &) = default;

private:
    Id m_id{};
    std::uint32_t m_width{};
    std::uint32_t m_height{};

    JobDrmFormat m_format;
    JobDrmModifier  m_modifier;
    Flags           m_flags{};

    Planes m_planes{};
    std::size_t m_planeCount{};
};

}