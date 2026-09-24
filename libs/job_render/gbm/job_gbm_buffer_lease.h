#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <gbm.h>

#include "job_gbm_buffer_plane.h"
#include "job_gbm_format.h"
#include "job_gbm_modifier.h"

namespace job::render {

class JobGbmBufferLease
{
public:
    using Ptr  = std::shared_ptr<JobGbmBufferLease>;
    using WPtr = std::weak_ptr<JobGbmBufferLease>;
    using UPtr = std::unique_ptr<JobGbmBufferLease>;

    using Width  = std::uint32_t;
    using Height = std::uint32_t;

    using Plane  = JobGbmBufferPlane;
    using Planes = std::vector<Plane>;

    struct Metadata
    {
        Width width{};
        Height height{};
        JobGbmFormat format;
        JobGbmModifier modifier;
        Planes planes;

        [[nodiscard]] bool valid() const noexcept
        {
            if (width == 0 || height == 0 || !format.valid() || planes.empty())
                return false;

            for (const auto &plane : planes) {
                if (!plane.valid())
                    return false;
            }

            return true;
        }
    };

    JobGbmBufferLease() = default;

    JobGbmBufferLease(gbm_surface *surface, gbm_bo *buffer)
    {
        (void)reset(surface, buffer);
    }

    ~JobGbmBufferLease();

    JobGbmBufferLease(const JobGbmBufferLease &) = delete;
    JobGbmBufferLease &operator=(const JobGbmBufferLease &) = delete;

    JobGbmBufferLease(JobGbmBufferLease &&other) noexcept;
    JobGbmBufferLease &operator=(JobGbmBufferLease &&other) noexcept;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGbmBufferLease>();
    }

    [[nodiscard]] static Ptr createShared(gbm_surface *surface, gbm_bo *buffer)
    {
        return std::make_shared<JobGbmBufferLease>(surface, buffer);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGbmBufferLease>();
    }

    [[nodiscard]] static UPtr createUniq(gbm_surface *surface, gbm_bo *buffer)
    {
        return std::make_unique<JobGbmBufferLease>(surface, buffer);
    }

    void reset() noexcept;
    [[nodiscard]] bool reset(gbm_surface *surface, gbm_bo *buffer);

    [[nodiscard]] gbm_bo *release() noexcept;

    [[nodiscard]] gbm_surface *surface() noexcept
    {
        return m_surface;
    }

    [[nodiscard]] const gbm_surface *surface() const noexcept
    {
        return m_surface;
    }

    [[nodiscard]] gbm_bo *buffer() noexcept
    {
        return m_buffer;
    }

    [[nodiscard]] const gbm_bo *buffer() const noexcept
    {
        return m_buffer;
    }

    [[nodiscard]] Width width() const noexcept
    {
        return m_metadata.width;
    }

    [[nodiscard]] Height height() const noexcept
    {
        return m_metadata.height;
    }

    [[nodiscard]] JobGbmFormat format() const noexcept
    {
        return m_metadata.format;
    }

    [[nodiscard]] JobGbmModifier modifier() const noexcept
    {
        return m_metadata.modifier;
    }

    [[nodiscard]] const Metadata &metadata() const noexcept
    {
        return m_metadata;
    }

    [[nodiscard]] std::span<const Plane> planes() const noexcept
    {
        return m_metadata.planes;
    }

    [[nodiscard]] std::size_t planeCount() const noexcept
    {
        return m_metadata.planes.size();
    }

    [[nodiscard]] const Plane *plane(std::size_t index) const noexcept
    {
        if (index >= m_metadata.planes.size())
            return nullptr;

        return &m_metadata.planes[index];
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_surface != nullptr && m_buffer != nullptr && m_metadata.valid();
    }

private:
    [[nodiscard]] bool populateMetadata();

    gbm_surface *m_surface{}; // BORROWED
    gbm_bo *m_buffer{};      // LEASED
    Metadata m_metadata;
};

}