#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <gbm.h>

#include "job_gbm_buffer_plane.h"
#include "job_gbm_format.h"
#include "job_gbm_modifier.h"

namespace job::render {

class JobGbmSurface
{
public:
    using Ptr  = std::shared_ptr<JobGbmSurface>;
    using WPtr = std::weak_ptr<JobGbmSurface>;
    using UPtr = std::unique_ptr<JobGbmSurface>;

    using Width  = std::uint32_t;
    using Height = std::uint32_t;
    using Usage  = std::uint32_t;

    JobGbmSurface() = default;
    ~JobGbmSurface() = default;

    JobGbmSurface(const JobGbmSurface &) = delete;
    JobGbmSurface &operator=(const JobGbmSurface &) = delete;

    JobGbmSurface(JobGbmSurface &&) = delete;
    JobGbmSurface &operator=(JobGbmSurface &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGbmSurface>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGbmSurface>();
    }

    [[nodiscard]] bool create(gbm_device *device, Width width, Height height, JobGbmFormat format, Usage usage);
    [[nodiscard]] bool create(gbm_device *device, Width width, Height height, JobGbmFormat format, std::span<const JobGbmModifier> modifiers, Usage usage);

    void reset() noexcept;

    [[nodiscard]] gbm_surface *surface() noexcept
    {
        return m_surface.get();
    }

    [[nodiscard]] const gbm_surface *surface() const noexcept
    {
        return m_surface.get();
    }

    [[nodiscard]] constexpr Width width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr Height height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] constexpr JobGbmFormat format() const noexcept
    {
        return m_format;
    }

    [[nodiscard]] constexpr Usage usage() const noexcept
    {
        return m_usage;
    }

    [[nodiscard]] bool hasFreeBuffers() const noexcept;

    [[nodiscard]] gbm_bo *lockFrontBuffer() noexcept;

    void releaseBuffer(gbm_bo *buffer) noexcept;

    [[nodiscard]] bool valid() const noexcept
    {
        return m_surface != nullptr;
    }

private:
    struct GbmSurfaceDeleter
    {
        void operator()(gbm_surface *surface) const noexcept;
    };
    using GbmSurfacePtr = std::unique_ptr<gbm_surface, GbmSurfaceDeleter>;

    GbmSurfacePtr   m_surface{};
    Width           m_width{};
    Height          m_height{};
    JobGbmFormat    m_format;
    Usage           m_usage{};
};

}