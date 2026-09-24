#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "job_render_format.h"

// buffer
#include "job_render_buffer_plane.h"
#include "job_render_buffer_usage.h"

namespace job::render {

class JobRenderBufferInfo
{
public:
    using Planes = std::vector<JobRenderBufferPlane>;

    JobRenderBufferInfo() = default;

    JobRenderBufferInfo( std::uint32_t width, std::uint32_t height, FormatType format, RenderBufferUsage usage, Planes planes = {}) :
        m_width(width),
        m_height(height),
        m_format(format),
        m_usage(usage),
        m_planes(std::move(planes))
    {
    }

    ~JobRenderBufferInfo() = default;

    JobRenderBufferInfo(const JobRenderBufferInfo &) = default;
    JobRenderBufferInfo &operator=(const JobRenderBufferInfo &) = default;

    JobRenderBufferInfo(JobRenderBufferInfo &&) noexcept = default;
    JobRenderBufferInfo &operator=(JobRenderBufferInfo &&) noexcept = default;

    [[nodiscard]] constexpr std::uint32_t width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr std::uint32_t height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] constexpr RenderFormat format() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] constexpr RenderBufferUsage usage() const noexcept
    {
        return m_usage;
    }

    [[nodiscard]] std::span<const JobRenderBufferPlane> planes() const noexcept
    {
        return m_planes;
    }

    [[nodiscard]] std::size_t planeCount() const noexcept
    {
        return m_planes.size();
    }

    [[nodiscard]] bool hasUsage(RenderBufferUsage usage) const noexcept
    {
        return job::render::hasUsage(m_usage, usage);
    }

    [[nodiscard]] bool valid() const noexcept
    {
        if (m_width == 0 || m_height == 0 || !validFormat(m_format))
            return false;

        for (const auto &plane : m_planes) {
            if (!plane.valid())
                return false;
        }

        return true;
    }

private:
    std::uint32_t       m_width{};
    std::uint32_t       m_height{};
    FormatType          m_format{FormatType::Unknown};
    RenderBufferUsage   m_usage{RenderBufferUsage::None};
    Planes              m_planes;
};

}