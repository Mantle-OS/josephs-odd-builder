#include "job_gbm_buffer.h"

namespace job::render {

void JobGbmBuffer::GbmBufferDeleter::operator()(gbm_bo *buffer) const noexcept
{
    if (buffer)
        gbm_bo_destroy(buffer);
}

bool JobGbmBuffer::create(gbm_device *device, Width width, Height height, JobGbmFormat format, Usage usage)
{
    reset();

    if (!device || width == 0 || height == 0 || !format.valid())
        return false;

    m_buffer.reset(gbm_bo_create(device,
                                 width,
                                 height,
                                 format.value(),
                                 usage));

    if (!m_buffer)
        return false;

    m_usage = usage;

    if (!populateMetadata()) {
        reset();
        return false;
    }

    return true;
}

bool JobGbmBuffer::create(gbm_device *device, Width width, Height height, JobGbmFormat format, std::span<const JobGbmModifier> modifiers, Usage usage)
{
    reset();

    if (!device || width == 0 || height == 0 || !format.valid() || modifiers.empty())
        return false;

    std::vector<std::uint64_t> values;
    values.reserve(modifiers.size());

    for (const auto &modifier : modifiers) {
        if (!modifier.valid())
            return false;

        values.push_back(modifier.value());
    }

    m_buffer.reset(gbm_bo_create_with_modifiers2(device,
                                                 width,
                                                 height,
                                                 format.value(),
                                                 values.data(),
                                                 values.size(),
                                                 usage));

    if (!m_buffer)
        return false;

    m_usage = usage;

    if (!populateMetadata()) {
        reset();
        return false;
    }

    return true;
}

void JobGbmBuffer::reset() noexcept
{
    m_buffer.reset();

    m_width = 0;
    m_height = 0;

    m_format = JobGbmFormat{};
    m_modifier = JobGbmModifier{};

    m_usage = 0;

    m_planes.clear();
}

bool JobGbmBuffer::populateMetadata()
{
    if (!m_buffer)
        return false;

    m_width    = gbm_bo_get_width(m_buffer.get());
    m_height   = gbm_bo_get_height(m_buffer.get());
    m_format   = JobGbmFormat{ gbm_bo_get_format(m_buffer.get()) };
    m_modifier = JobGbmModifier{ gbm_bo_get_modifier(m_buffer.get()) };

    const int planeCount = gbm_bo_get_plane_count(m_buffer.get());
    if (planeCount <= 0)
        return false;

    m_planes.clear();
    m_planes.reserve(static_cast<std::size_t>(planeCount));

    for (int i = 0; i < planeCount; ++i) {
        const gbm_bo_handle handle = gbm_bo_get_handle_for_plane(m_buffer.get(), i);
        const std::uint32_t stride = gbm_bo_get_stride_for_plane(m_buffer.get(), i);
        const std::uint32_t offset = gbm_bo_get_offset(m_buffer.get(), i);
        m_planes.emplace_back( static_cast<JobGbmBufferPlane::Index>(i), handle.u32, stride, offset);
    }

    return true;
}

}