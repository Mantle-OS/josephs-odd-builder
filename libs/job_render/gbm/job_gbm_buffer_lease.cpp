#include "job_gbm_buffer_lease.h"

#include <utility>

namespace job::render {

JobGbmBufferLease::~JobGbmBufferLease()
{
    reset();
}

JobGbmBufferLease::JobGbmBufferLease(JobGbmBufferLease &&other) noexcept :
    m_surface(std::exchange(other.m_surface, nullptr)),
    m_buffer(std::exchange(other.m_buffer, nullptr)),
    m_metadata(std::move(other.m_metadata))
{
    other.m_metadata = {};
}

JobGbmBufferLease &JobGbmBufferLease::operator=(JobGbmBufferLease &&other) noexcept
{
    if (this == &other)
        return *this;

    reset();

    m_surface = std::exchange(other.m_surface, nullptr);
    m_buffer = std::exchange(other.m_buffer, nullptr);
    m_metadata = std::move(other.m_metadata);

    other.m_metadata = {};

    return *this;
}

void JobGbmBufferLease::reset() noexcept
{
    if (m_surface && m_buffer)
        gbm_surface_release_buffer(m_surface, m_buffer);

    m_surface = nullptr;
    m_buffer = nullptr;
    m_metadata = {};
}

bool JobGbmBufferLease::reset(gbm_surface *surface, gbm_bo *buffer)
{
    reset();

    if (!surface || !buffer)
        return false;

    m_surface = surface;
    m_buffer = buffer;

    if (!populateMetadata()) {
        reset();
        return false;
    }

    return true;
}

gbm_bo *JobGbmBufferLease::release() noexcept
{
    gbm_bo *buffer = m_buffer;

    m_surface = nullptr;
    m_buffer = nullptr;
    m_metadata = {};

    return buffer;
}

bool JobGbmBufferLease::populateMetadata()
{
    if (!m_buffer)
        return false;

    Metadata metadata;

    metadata.width = gbm_bo_get_width(m_buffer);
    metadata.height = gbm_bo_get_height(m_buffer);
    metadata.format = JobGbmFormat{gbm_bo_get_format(m_buffer)};
    metadata.modifier = JobGbmModifier{gbm_bo_get_modifier(m_buffer)};

    const int planeCount = gbm_bo_get_plane_count(m_buffer);
    if (planeCount <= 0)
        return false;

    metadata.planes.reserve(static_cast<std::size_t>(planeCount));

    for (int i = 0; i < planeCount; ++i) {
        const gbm_bo_handle handle = gbm_bo_get_handle_for_plane(m_buffer, i);
        const std::uint32_t stride = gbm_bo_get_stride_for_plane(m_buffer, i);
        const std::uint32_t offset = gbm_bo_get_offset(m_buffer, i);

        if (handle.u32 == 0 || stride == 0)
            return false;

        metadata.planes.emplace_back(static_cast<Plane::Index>(i), handle.u32, stride, offset);
    }

    if (!metadata.valid())
        return false;

    m_metadata = std::move(metadata);

    return true;
}

}
