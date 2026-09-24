#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include "job_render_buffer_info.h"

namespace job::render {

class JobRenderBuffer
{
public:
    using Ptr  = std::shared_ptr<JobRenderBuffer>;
    using WPtr = std::weak_ptr<JobRenderBuffer>;
    using UPtr = std::unique_ptr<JobRenderBuffer>;

    JobRenderBuffer() = default;
    virtual ~JobRenderBuffer() = default;

    JobRenderBuffer(const JobRenderBuffer &) = delete;
    JobRenderBuffer &operator=(const JobRenderBuffer &) = delete;

    JobRenderBuffer(JobRenderBuffer &&) = delete;
    JobRenderBuffer &operator=(JobRenderBuffer &&) = delete;

    [[nodiscard]] virtual const JobRenderBufferInfo &info() const noexcept = 0;
    [[nodiscard]] virtual bool valid() const noexcept = 0;

    [[nodiscard]] std::uint32_t width() const noexcept
    {
        return info().width();
    }

    [[nodiscard]] std::uint32_t height() const noexcept
    {
        return info().height();
    }

    [[nodiscard]] FormatType format() const noexcept
    {
        return info().format();
    }

    [[nodiscard]] RenderBufferUsage usage() const noexcept
    {
        return info().usage();
    }

    [[nodiscard]] std::span<const JobRenderBufferPlane> planes() const noexcept
    {
        return info().planes();
    }

    [[nodiscard]] std::size_t planeCount() const noexcept
    {
        return info().planeCount();
    }

    [[nodiscard]] bool hasUsage(RenderBufferUsage usage) const noexcept
    {
        return info().hasUsage(usage);
    }
};

}