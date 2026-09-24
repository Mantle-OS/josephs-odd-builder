#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <linux/fb.h>

#include "job_fb_bitfield.h"

namespace job::render {

class JobFbVariableInfo
{
public:
    using Ptr  = std::shared_ptr<JobFbVariableInfo>;
    using WPtr = std::weak_ptr<JobFbVariableInfo>;
    using UPtr = std::unique_ptr<JobFbVariableInfo>;

    JobFbVariableInfo() = default;
    ~JobFbVariableInfo() = default;

    JobFbVariableInfo(const JobFbVariableInfo &) = default;
    JobFbVariableInfo &operator=(const JobFbVariableInfo &) = default;

    JobFbVariableInfo(JobFbVariableInfo &&) noexcept = default;
    JobFbVariableInfo &operator=(JobFbVariableInfo &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobFbVariableInfo>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobFbVariableInfo>();
    }

    [[nodiscard]] static JobFbVariableInfo fromFb(const fb_var_screeninfo &info) noexcept;
    [[nodiscard]] fb_var_screeninfo toFb() const noexcept;

    [[nodiscard]] constexpr std::uint32_t width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr std::uint32_t height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] constexpr std::uint32_t virtualWidth() const noexcept
    {
        return m_virtualWidth;
    }

    [[nodiscard]] constexpr std::uint32_t virtualHeight() const noexcept
    {
        return m_virtualHeight;
    }

    [[nodiscard]] constexpr std::uint32_t xOffset() const noexcept
    {
        return m_xOffset;
    }

    [[nodiscard]] constexpr std::uint32_t yOffset() const noexcept
    {
        return m_yOffset;
    }

    [[nodiscard]] constexpr std::uint32_t bitsPerPixel() const noexcept
    {
        return m_bitsPerPixel;
    }

    [[nodiscard]] constexpr std::uint32_t grayscale() const noexcept
    {
        return m_grayscale;
    }

    [[nodiscard]] constexpr bool color() const noexcept
    {
        return m_grayscale == 0;
    }

    [[nodiscard]] constexpr bool grayscaleMode() const noexcept
    {
        return m_grayscale == 1;
    }

    [[nodiscard]] constexpr bool fourCcMode() const noexcept
    {
        return m_grayscale > 1;
    }

    [[nodiscard]] constexpr std::uint32_t fourCc() const noexcept
    {
        return fourCcMode() ? m_grayscale : 0;
    }

    [[nodiscard]] constexpr const JobFbBitfield &red() const noexcept
    {
        return m_red;
    }

    [[nodiscard]] constexpr const JobFbBitfield &green() const noexcept
    {
        return m_green;
    }

    [[nodiscard]] constexpr const JobFbBitfield &blue() const noexcept
    {
        return m_blue;
    }

    [[nodiscard]] constexpr const JobFbBitfield &alpha() const noexcept
    {
        return m_alpha;
    }

    [[nodiscard]] constexpr std::uint32_t nonStandard() const noexcept
    {
        return m_nonStandard;
    }

    [[nodiscard]] constexpr std::uint32_t activate() const noexcept
    {
        return m_activate;
    }

    [[nodiscard]] constexpr std::uint32_t physicalHeightMm() const noexcept
    {
        return m_physicalHeightMm;
    }

    [[nodiscard]] constexpr std::uint32_t physicalWidthMm() const noexcept
    {
        return m_physicalWidthMm;
    }

    [[nodiscard]] constexpr std::uint32_t accelerationFlags() const noexcept
    {
        return m_accelerationFlags;
    }

    [[nodiscard]] constexpr std::uint32_t pixelClockPs() const noexcept
    {
        return m_pixelClockPs;
    }

    [[nodiscard]] constexpr std::uint32_t leftMargin() const noexcept
    {
        return m_leftMargin;
    }

    [[nodiscard]] constexpr std::uint32_t rightMargin() const noexcept
    {
        return m_rightMargin;
    }

    [[nodiscard]] constexpr std::uint32_t upperMargin() const noexcept
    {
        return m_upperMargin;
    }

    [[nodiscard]] constexpr std::uint32_t lowerMargin() const noexcept
    {
        return m_lowerMargin;
    }

    [[nodiscard]] constexpr std::uint32_t hsyncLength() const noexcept
    {
        return m_hsyncLength;
    }

    [[nodiscard]] constexpr std::uint32_t vsyncLength() const noexcept
    {
        return m_vsyncLength;
    }

    [[nodiscard]] constexpr std::uint32_t sync() const noexcept
    {
        return m_sync;
    }

    [[nodiscard]] constexpr std::uint32_t videoMode() const noexcept
    {
        return m_videoMode;
    }

    [[nodiscard]] constexpr std::uint32_t rotation() const noexcept
    {
        return m_rotation;
    }

    [[nodiscard]] constexpr std::uint32_t colorSpace() const noexcept
    {
        return m_colorSpace;
    }

    [[nodiscard]] constexpr const std::array<std::uint32_t, 4> &reserved() const noexcept
    {
        return m_reserved;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_width != 0 && m_height != 0 && m_virtualWidth >= m_width && m_virtualHeight >= m_height && m_bitsPerPixel != 0;
    }

private:
    std::uint32_t                   m_width{};
    std::uint32_t                   m_height{};
    std::uint32_t                   m_virtualWidth{};
    std::uint32_t                   m_virtualHeight{};
    std::uint32_t                   m_xOffset{};
    std::uint32_t                   m_yOffset{};
    std::uint32_t                   m_bitsPerPixel{};
    std::uint32_t                   m_grayscale{};
    JobFbBitfield                   m_red;
    JobFbBitfield                   m_green;
    JobFbBitfield                   m_blue;
    JobFbBitfield                   m_alpha;
    std::uint32_t                   m_nonStandard{};
    std::uint32_t                   m_activate{};
    std::uint32_t                   m_physicalHeightMm{};
    std::uint32_t                   m_physicalWidthMm{};
    std::uint32_t                   m_accelerationFlags{};
    std::uint32_t                   m_pixelClockPs{};
    std::uint32_t                   m_leftMargin{};
    std::uint32_t                   m_rightMargin{};
    std::uint32_t                   m_upperMargin{};
    std::uint32_t                   m_lowerMargin{};
    std::uint32_t                   m_hsyncLength{};
    std::uint32_t                   m_vsyncLength{};
    std::uint32_t                   m_sync{};
    std::uint32_t                   m_videoMode{};
    std::uint32_t                   m_rotation{};
    std::uint32_t                   m_colorSpace{};
    std::array<std::uint32_t, 4>    m_reserved{};
};

}