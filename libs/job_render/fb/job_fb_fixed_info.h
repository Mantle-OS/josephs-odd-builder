#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include <linux/fb.h>

#include "job_fb_capabilities.h"
#include "job_fb_type.h"
#include "job_fb_visual.h"

namespace job::render {

class JobFbFixedInfo
{
public:
    using Ptr  = std::shared_ptr<JobFbFixedInfo>;
    using WPtr = std::weak_ptr<JobFbFixedInfo>;
    using UPtr = std::unique_ptr<JobFbFixedInfo>;

    using MemoryAddress = std::uint64_t;
    using MemoryLength  = std::uint32_t;
    using LineLength    = std::uint32_t;

    JobFbFixedInfo() = default;
    ~JobFbFixedInfo() = default;

    JobFbFixedInfo(const JobFbFixedInfo &) = default;
    JobFbFixedInfo &operator=(const JobFbFixedInfo &) = default;

    JobFbFixedInfo(JobFbFixedInfo &&) noexcept = default;
    JobFbFixedInfo &operator=(JobFbFixedInfo &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobFbFixedInfo>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobFbFixedInfo>();
    }

    [[nodiscard]] static JobFbFixedInfo fromFb(const fb_fix_screeninfo &info);

    [[nodiscard]] fb_fix_screeninfo toFb() const noexcept;

    [[nodiscard]] const std::string &id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr MemoryAddress memoryStart() const noexcept
    {
        return m_memoryStart;
    }

    [[nodiscard]] constexpr MemoryLength memoryLength() const noexcept
    {
        return m_memoryLength;
    }

    [[nodiscard]] constexpr JobFbType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr std::uint32_t typeAux() const noexcept
    {
        return m_typeAux;
    }

    [[nodiscard]] constexpr JobFbVisual visual() const noexcept
    {
        return m_visual;
    }

    [[nodiscard]] constexpr std::uint16_t xPanStep() const noexcept
    {
        return m_xPanStep;
    }

    [[nodiscard]] constexpr std::uint16_t yPanStep() const noexcept
    {
        return m_yPanStep;
    }

    [[nodiscard]] constexpr std::uint16_t yWrapStep() const noexcept
    {
        return m_yWrapStep;
    }

    [[nodiscard]] constexpr LineLength lineLength() const noexcept
    {
        return m_lineLength;
    }

    [[nodiscard]] constexpr MemoryAddress mmioStart() const noexcept
    {
        return m_mmioStart;
    }

    [[nodiscard]] constexpr MemoryLength mmioLength() const noexcept
    {
        return m_mmioLength;
    }

    [[nodiscard]] constexpr std::uint32_t acceleration() const noexcept
    {
        return m_acceleration;
    }

    [[nodiscard]] constexpr JobFbCapabilities capabilities() const noexcept
    {
        return m_capabilities;
    }

    [[nodiscard]] constexpr const std::array<std::uint16_t, 2> &reserved() const noexcept
    {
        return m_reserved;
    }

    [[nodiscard]] constexpr bool canPanX() const noexcept
    {
        return m_xPanStep != 0;
    }

    [[nodiscard]] constexpr bool canPanY() const noexcept
    {
        return m_yPanStep != 0;
    }

    [[nodiscard]] constexpr bool canWrapY() const noexcept
    {
        return m_yWrapStep != 0;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_memoryLength != 0 && m_lineLength != 0;
    }

private:
    std::string m_id;
    MemoryAddress                   m_memoryStart{};
    MemoryLength                    m_memoryLength{};
    JobFbType                       m_type;
    std::uint32_t                   m_typeAux{};
    JobFbVisual                     m_visual;
    std::uint16_t                   m_xPanStep{};
    std::uint16_t                   m_yPanStep{};
    std::uint16_t                   m_yWrapStep{};
    LineLength                      m_lineLength{};
    MemoryAddress                   m_mmioStart{};
    MemoryLength                    m_mmioLength{};
    std::uint32_t                   m_acceleration{};
    JobFbCapabilities               m_capabilities;
    std::array<std::uint16_t, 2>    m_reserved{};
};

}