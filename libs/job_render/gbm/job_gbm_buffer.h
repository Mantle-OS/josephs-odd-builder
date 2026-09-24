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

class JobGbmBuffer
{
public:
    using Ptr  = std::shared_ptr<JobGbmBuffer>;
    using WPtr = std::weak_ptr<JobGbmBuffer>;
    using UPtr = std::unique_ptr<JobGbmBuffer>;

    using Width  = std::uint32_t;
    using Height = std::uint32_t;
    using Usage  = std::uint32_t;

    using Plane  = JobGbmBufferPlane;
    using Planes = std::vector<Plane>;

    JobGbmBuffer() = default;
    ~JobGbmBuffer() = default;

    JobGbmBuffer(const JobGbmBuffer &) = delete;
    JobGbmBuffer &operator=(const JobGbmBuffer &) = delete;

    JobGbmBuffer(JobGbmBuffer &&) = delete;
    JobGbmBuffer &operator=(JobGbmBuffer &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGbmBuffer>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGbmBuffer>();
    }

    [[nodiscard]] bool create(gbm_device *device, Width width, Height height, JobGbmFormat format, Usage usage);
    [[nodiscard]] bool create(gbm_device *device, Width width, Height height, JobGbmFormat format, std::span<const JobGbmModifier> modifiers, Usage usage);

    void reset() noexcept;

    [[nodiscard]] gbm_bo *buffer() noexcept
    {
        return m_buffer.get();
    }

    [[nodiscard]] const gbm_bo *buffer() const noexcept
    {
        return m_buffer.get();
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

    [[nodiscard]] constexpr JobGbmModifier modifier() const noexcept
    {
        return m_modifier;
    }

    [[nodiscard]] constexpr Usage usage() const noexcept
    {
        return m_usage;
    }

    [[nodiscard]] std::span<const Plane> planes() const noexcept
    {
        return m_planes;
    }

    [[nodiscard]] std::size_t planeCount() const noexcept
    {
        return m_planes.size();
    }

    [[nodiscard]] const Plane *plane(std::size_t index) const noexcept
    {
        if (index >= m_planes.size())
            return nullptr;

        return &m_planes[index];
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_buffer != nullptr;
    }

private:
    [[nodiscard]] bool populateMetadata();
    struct GbmBufferDeleter
    {
        void operator()(gbm_bo *buffer) const noexcept;
    };

    using GbmBufferPtr = std::unique_ptr<gbm_bo, GbmBufferDeleter>;

    GbmBufferPtr    m_buffer{};

    Width           m_width{};
    Height          m_height{};

    JobGbmFormat    m_format;
    JobGbmModifier  m_modifier;

    Usage           m_usage{};
    Planes          m_planes;
};

}