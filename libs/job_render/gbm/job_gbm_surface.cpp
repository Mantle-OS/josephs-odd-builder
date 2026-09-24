#include "job_gbm_surface.h"

namespace job::render {

void JobGbmSurface::GbmSurfaceDeleter::operator()(gbm_surface *surface) const noexcept
{
    if (surface)
        gbm_surface_destroy(surface);
}

bool JobGbmSurface::create(gbm_device *device, Width width, Height height, JobGbmFormat format, Usage usage)
{
    reset();

    if (!device || width == 0 || height == 0 || !format.valid())
        return false;

    m_surface.reset(gbm_surface_create(device, width, height, format.value(), usage));

    if (!m_surface)
        return false;

    m_width = width;
    m_height = height;
    m_format = format;
    m_usage = usage;

    return true;
}

bool JobGbmSurface::create(gbm_device *device, Width width, Height height, JobGbmFormat format, std::span<const JobGbmModifier> modifiers, Usage usage)
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

    m_surface.reset(gbm_surface_create_with_modifiers2(device,
                                                       width,
                                                       height,
                                                       format.value(),
                                                       values.data(),
                                                       static_cast<unsigned int>(values.size()),
                                                       usage));

    if (!m_surface)
        return false;

    m_width = width;
    m_height = height;
    m_format = format;
    m_usage = usage;

    return true;
}

void JobGbmSurface::reset() noexcept
{
    m_surface.reset();

    m_width = 0;
    m_height = 0;

    m_format = JobGbmFormat{};

    m_usage = 0;
}

bool JobGbmSurface::hasFreeBuffers() const noexcept
{
    if (!m_surface)
        return false;

    return gbm_surface_has_free_buffers(m_surface.get()) != 0;
}

gbm_bo *JobGbmSurface::lockFrontBuffer() noexcept
{
    if (!m_surface)
        return nullptr;

    return gbm_surface_lock_front_buffer(m_surface.get());
}

void JobGbmSurface::releaseBuffer(gbm_bo *buffer) noexcept
{
    if (!m_surface || !buffer)
        return;

    gbm_surface_release_buffer(m_surface.get(), buffer);
}

}