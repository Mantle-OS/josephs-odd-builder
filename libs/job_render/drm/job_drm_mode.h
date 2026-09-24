#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <algorithm>
#include <cstddef>
#include <cstring>

#include <drm_mode.h>
#include <xf86drmMode.h>

namespace job::render {

class JobDrmMode
{
public:
    using Ptr  = std::shared_ptr<JobDrmMode>;
    using WPtr = std::weak_ptr<JobDrmMode>;
    using UPtr = std::unique_ptr<JobDrmMode>;

    using Flags     = std::uint32_t;
    using Type      = std::uint32_t;

    JobDrmMode() = default;

    JobDrmMode(std::uint32_t clock,
               std::uint16_t hdisplay, std::uint16_t hsyncStart, std::uint16_t hsyncEnd, std::uint16_t htotal, std::uint16_t hskew,
               std::uint16_t vdisplay, std::uint16_t vsyncStart, std::uint16_t vsyncEnd, std::uint16_t vtotal,
               std::uint16_t vscan, std::uint32_t vrefresh,
               Flags flags, Type type, std::string name) :
        m_clock(clock),
        m_hdisplay(hdisplay),
        m_hsyncStart(hsyncStart),
        m_hsyncEnd(hsyncEnd),
        m_htotal(htotal),
        m_hskew(hskew),
        m_vdisplay(vdisplay),
        m_vsyncStart(vsyncStart),
        m_vsyncEnd(vsyncEnd),
        m_vtotal(vtotal),
        m_vscan(vscan),
        m_vrefresh(vrefresh),
        m_flags(flags),
        m_type(type),
        m_name(std::move(name))
    {
    }

    ~JobDrmMode() = default;

    JobDrmMode(const JobDrmMode &) = default;
    JobDrmMode &operator=(const JobDrmMode &) = default;

    JobDrmMode(JobDrmMode &&) noexcept = default;
    JobDrmMode &operator=(JobDrmMode &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(std::uint32_t clock,
                                          std::uint16_t hdisplay, std::uint16_t hsyncStart, std::uint16_t hsyncEnd, std::uint16_t htotal, std::uint16_t hskew,
                                          std::uint16_t vdisplay, std::uint16_t vsyncStart, std::uint16_t vsyncEnd, std::uint16_t vtotal, std::uint16_t vscan, std::uint32_t vrefresh,
                                          Flags flags, Type type, std::string name)
    {
        return std::make_shared<JobDrmMode>(clock,
                                            hdisplay, hsyncStart, hsyncEnd, htotal, hskew,
                                            vdisplay, vsyncStart, vsyncEnd, vtotal, vscan, vrefresh,
                                            flags, type,
                                            std::move(name));
    }

    [[nodiscard]] static UPtr createUniq(std::uint32_t clock,
                                         std::uint16_t hdisplay, std::uint16_t hsyncStart, std::uint16_t hsyncEnd, std::uint16_t htotal, std::uint16_t hskew,
                                         std::uint16_t vdisplay, std::uint16_t vsyncStart, std::uint16_t vsyncEnd, std::uint16_t vtotal, std::uint16_t vscan, std::uint32_t vrefresh,
                                         Flags flags, Type type, std::string name)
    {
        return std::make_unique<JobDrmMode>(clock,
                                            hdisplay, hsyncStart, hsyncEnd, htotal, hskew,
                                            vdisplay, vsyncStart, vsyncEnd, vtotal, vscan, vrefresh,
                                            flags, type, std::move(name));
    }

    [[nodiscard]] static JobDrmMode fromDrm(const drmModeModeInfo &mode)
    {
        return JobDrmMode{
            mode.clock,
            mode.hdisplay,
            mode.hsync_start,
            mode.hsync_end,
            mode.htotal,
            mode.hskew,
            mode.vdisplay,
            mode.vsync_start,
            mode.vsync_end,
            mode.vtotal,
            mode.vscan,
            mode.vrefresh,
            mode.flags,
            mode.type,
            mode.name
        };
    }
    // Add toDrm
    [[nodiscard]] drmModeModeInfo toDrm() const noexcept
    {
        drmModeModeInfo mode{};
        mode.clock          = m_clock;
        mode.hdisplay       = m_hdisplay;
        mode.hsync_start    = m_hsyncStart;
        mode.hsync_end      = m_hsyncEnd;
        mode.htotal         = m_htotal;
        mode.hskew          = m_hskew;
        mode.vdisplay       = m_vdisplay;
        mode.vsync_start    = m_vsyncStart;
        mode.vsync_end      = m_vsyncEnd;
        mode.vtotal         = m_vtotal;
        mode.vscan          = m_vscan;
        mode.vrefresh       = m_vrefresh;
        mode.flags          = m_flags;
        mode.type           = m_type;

        const std::size_t length = std::min( m_name.size(), sizeof(mode.name) - 1);
        std::memcpy(mode.name, m_name.data(), length);
        mode.name[length]   = '\0';

        return mode;
    }

    [[nodiscard]] constexpr std::uint32_t clock() const noexcept
    {
        return m_clock;
    }

    [[nodiscard]] constexpr std::uint16_t width() const noexcept
    {
        return m_hdisplay;
    }

    [[nodiscard]] constexpr std::uint16_t height() const noexcept
    {
        return m_vdisplay;
    }

    [[nodiscard]] constexpr std::uint16_t hdisplay() const noexcept
    {
        return m_hdisplay;
    }

    [[nodiscard]] constexpr std::uint16_t hsyncStart() const noexcept
    {
        return m_hsyncStart;
    }

    [[nodiscard]] constexpr std::uint16_t hsyncEnd() const noexcept
    {
        return m_hsyncEnd;
    }

    [[nodiscard]] constexpr std::uint16_t htotal() const noexcept
    {
        return m_htotal;
    }

    [[nodiscard]] constexpr std::uint16_t hskew() const noexcept
    {
        return m_hskew;
    }

    [[nodiscard]] constexpr std::uint16_t vdisplay() const noexcept
    {
        return m_vdisplay;
    }

    [[nodiscard]] constexpr std::uint16_t vsyncStart() const noexcept
    {
        return m_vsyncStart;
    }

    [[nodiscard]] constexpr std::uint16_t vsyncEnd() const noexcept
    {
        return m_vsyncEnd;
    }

    [[nodiscard]] constexpr std::uint16_t vtotal() const noexcept
    {
        return m_vtotal;
    }

    [[nodiscard]] constexpr std::uint16_t vscan() const noexcept
    {
        return m_vscan;
    }

    [[nodiscard]] constexpr std::uint32_t refreshRate() const noexcept
    {
        return m_vrefresh;
    }

    [[nodiscard]] constexpr Flags flags() const noexcept
    {
        return m_flags;
    }

    [[nodiscard]] constexpr Type type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] std::string_view name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_hdisplay != 0 && m_vdisplay != 0;
    }

    [[nodiscard]] constexpr bool preferred() const noexcept
    {
        return (m_type & DRM_MODE_TYPE_PREFERRED) != 0;
    }

    [[nodiscard]] constexpr bool driver() const noexcept
    {
        return (m_type & DRM_MODE_TYPE_DRIVER) != 0;
    }

    [[nodiscard]] constexpr bool userDefined() const noexcept
    {
        return (m_type & DRM_MODE_TYPE_USERDEF) != 0;
    }

    [[nodiscard]] constexpr bool interlaced() const noexcept
    {
        return (m_flags & DRM_MODE_FLAG_INTERLACE) != 0;
    }

    [[nodiscard]] constexpr bool doubleScan() const noexcept
    {
        return (m_flags & DRM_MODE_FLAG_DBLSCAN) != 0;
    }

    [[nodiscard]] constexpr bool positiveHSync() const noexcept
    {
        return (m_flags & DRM_MODE_FLAG_PHSYNC) != 0;
    }

    [[nodiscard]] constexpr bool negativeHSync() const noexcept
    {
        return (m_flags & DRM_MODE_FLAG_NHSYNC) != 0;
    }

    [[nodiscard]] constexpr bool positiveVSync() const noexcept
    {
        return (m_flags & DRM_MODE_FLAG_PVSYNC) != 0;
    }

    [[nodiscard]] constexpr bool negativeVSync() const noexcept
    {
        return (m_flags & DRM_MODE_FLAG_NVSYNC) != 0;
    }

    [[nodiscard]] constexpr bool hasFlag(Flags flag) const noexcept
    {
        return (m_flags & flag) == flag;
    }

    [[nodiscard]] constexpr bool hasType(Type type) const noexcept
    {
        return (m_type & type) == type;
    }

    [[nodiscard]] friend bool operator==(const JobDrmMode &, const JobDrmMode &) = default;

private:
    std::uint32_t   m_clock{};

    std::uint16_t   m_hdisplay{};
    std::uint16_t   m_hsyncStart{};
    std::uint16_t   m_hsyncEnd{};
    std::uint16_t   m_htotal{};
    std::uint16_t   m_hskew{};

    std::uint16_t   m_vdisplay{};
    std::uint16_t   m_vsyncStart{};
    std::uint16_t   m_vsyncEnd{};
    std::uint16_t   m_vtotal{};
    std::uint16_t   m_vscan{};

    std::uint32_t   m_vrefresh{};

    Flags           m_flags{};
    Type            m_type{};

    std::string     m_name;
};

}