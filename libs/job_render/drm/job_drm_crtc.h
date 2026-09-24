#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include <xf86drmMode.h>

#include "job_drm_mode.h"
#include "job_drm_object_properties.h"

namespace job::render {

class JobDrmCrtc
{
public:
    using Ptr  = std::shared_ptr<JobDrmCrtc>;
    using WPtr = std::weak_ptr<JobDrmCrtc>;
    using UPtr = std::unique_ptr<JobDrmCrtc>;

    using Id            = std::uint32_t;
    using FramebufferId = std::uint32_t;

    JobDrmCrtc() = default;

    JobDrmCrtc(Id id,
               FramebufferId framebufferId,
               std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height,
               bool modeValid,
               JobDrmMode mode,
               std::uint32_t gammaSize,
               JobDrmObjectProperties properties) :
        m_id(id),
        m_framebufferId(framebufferId),
        m_x(x),
        m_y(y),
        m_width(width),
        m_height(height),
        m_modeValid(modeValid),
        m_mode(std::move(mode)),
        m_gammaSize(gammaSize),
        m_properties(std::move(properties))
    {
    }

    ~JobDrmCrtc() = default;

    JobDrmCrtc(const JobDrmCrtc &) = default;
    JobDrmCrtc &operator=(const JobDrmCrtc &) = default;

    JobDrmCrtc(JobDrmCrtc &&) noexcept = default;
    JobDrmCrtc &operator=(JobDrmCrtc &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          FramebufferId framebufferId,
                                          std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height,
                                          bool modeValid,
                                          JobDrmMode mode,
                                          std::uint32_t gammaSize,
                                          JobDrmObjectProperties properties)
    {
        return std::make_shared<JobDrmCrtc>(id,
                                            framebufferId,
                                            x, y, width, height,
                                            modeValid,
                                            std::move(mode),
                                            gammaSize,
                                            std::move(properties));
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         FramebufferId framebufferId,
                                         std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height,
                                         bool modeValid,
                                         JobDrmMode mode,
                                         std::uint32_t gammaSize,
                                         JobDrmObjectProperties properties)
    {
        return std::make_unique<JobDrmCrtc>(id,
                                            framebufferId,
                                            x, y, width, height,
                                            modeValid,
                                            std::move(mode),
                                            gammaSize,
                                            std::move(properties));
    }

    [[nodiscard]] static JobDrmCrtc fromDrm(const drmModeCrtc &crtc, JobDrmObjectProperties properties)
    {
        return JobDrmCrtc{
            crtc.crtc_id,
            crtc.buffer_id,
            crtc.x,
            crtc.y,
            crtc.width,
            crtc.height,
            crtc.mode_valid != 0,
            crtc.mode_valid != 0 ? JobDrmMode::fromDrm(crtc.mode) : JobDrmMode{},
            static_cast<std::uint32_t>(crtc.gamma_size),
            std::move(properties)
        };
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr FramebufferId framebufferId() const noexcept
    {
        return m_framebufferId;
    }

    [[nodiscard]] constexpr std::uint32_t x() const noexcept
    {
        return m_x;
    }

    [[nodiscard]] constexpr std::uint32_t y() const noexcept
    {
        return m_y;
    }

    [[nodiscard]] constexpr std::uint32_t width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr std::uint32_t height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] constexpr bool modeValid() const noexcept
    {
        return m_modeValid;
    }

    [[nodiscard]] const JobDrmMode &mode() const noexcept
    {
        return m_mode;
    }

    [[nodiscard]] constexpr std::uint32_t gammaSize() const noexcept
    {
        return m_gammaSize;
    }

    [[nodiscard]] const JobDrmObjectProperties &properties() const noexcept
    {
        return m_properties;
    }

    [[nodiscard]] constexpr bool active() const noexcept
    {
        return m_modeValid;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    [[nodiscard]] friend bool operator==(const JobDrmCrtc &, const JobDrmCrtc &) = default;

private:
    Id                      m_id{};
    FramebufferId           m_framebufferId{};
    std::uint32_t           m_x{};
    std::uint32_t           m_y{};
    std::uint32_t           m_width{};
    std::uint32_t           m_height{};
    bool                    m_modeValid{};
    JobDrmMode              m_mode;
    std::uint32_t           m_gammaSize{};
    JobDrmObjectProperties  m_properties;
};

}