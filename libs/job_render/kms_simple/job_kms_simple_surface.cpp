#include "job_kms_simple_surface.h"

#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <drm/drm.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <job_logger.h>

namespace job::render {

JobKmsSimpleSurface::JobKmsSimpleSurface(JobDrmSimpleDevice::Ptr device) :
    m_device(std::move(device))
{
}

JobKmsSimpleSurface::~JobKmsSimpleSurface()
{
    destroy();
}

JobKmsSimpleSurface::Ptr JobKmsSimpleSurface::createShared(JobDrmSimpleDevice::Ptr device)
{
    return std::make_shared<JobKmsSimpleSurface>(std::move(device));
}

JobKmsSimpleSurface::UPtr JobKmsSimpleSurface::createUniq(JobDrmSimpleDevice::Ptr device)
{
    return std::make_unique<JobKmsSimpleSurface>(std::move(device));
}

bool JobKmsSimpleSurface::create(std::uint32_t width, std::uint32_t height, FormatType format)
{
    destroy();

    if (!m_device || !m_device->isOpen()) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] DRM device is not valid or open.");
        return false;
    }

    if (width == 0 || height == 0) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Invalid surface size {}x{}.", width, height);
        return false;
    }

    if (!dumbValidFormats(format)) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Invalid render format.");
        return false;
    }

    if (dumbFramebufferFormat(format) == DRM_FORMAT_INVALID) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Render format is not supported by KMS surface.");
        return false;
    }

    m_width     = width;
    m_height    = height;
    m_format    = format;

    if (!createDumbBuffer()) {
        destroy();
        return false;
    }

    if (!createFramebuffer()) {
        destroy();
        return false;
    }

    if (!map()) {
        destroy();
        return false;
    }

    return true;
}

void JobKmsSimpleSurface::destroy()
{

    if (!m_device || !m_device->isOpen()) {
        m_data              = nullptr;
        m_handle            = 0;
        m_framebufferId     = 0;
        m_width             = 0;
        m_height            = 0;
        m_stride            = 0;
        m_size              = 0;
        m_format            = FormatType::Unknown;
        return;
    }

    unmap();

    if (m_framebufferId != 0) {
        if (drmModeRmFB(m_device->fd(), m_framebufferId) != 0) {
            JOB_LOG_WARN("[JobKmsSimpleSurface] Failed to remove framebuffer {}: {}", m_framebufferId, std::strerror(errno));
        }

        m_framebufferId = 0;
    }

    if (m_handle != 0) {
        drm_mode_destroy_dumb destroy{};
        destroy.handle = m_handle;
        if (::ioctl(m_device->fd(), DRM_IOCTL_MODE_DESTROY_DUMB, &destroy) < 0) {
            JOB_LOG_WARN("[JobKmsSimpleSurface] Failed to destroy dumb buffer {}: {}", m_handle, std::strerror(errno));
        }

        m_handle = 0;
    }

    m_width     = 0;
    m_height    = 0;
    m_stride    = 0;
    m_size      = 0;
    m_format    = FormatType::Unknown;
}

std::uint32_t JobKmsSimpleSurface::width() const noexcept
{
    return m_width;
}

std::uint32_t JobKmsSimpleSurface::height() const noexcept
{
    return m_height;
}

FormatType JobKmsSimpleSurface::format() const noexcept
{
    return m_format;
}

std::size_t JobKmsSimpleSurface::stride() const noexcept
{
    return m_stride;
}

std::size_t JobKmsSimpleSurface::size() const noexcept
{
    return m_size;
}

std::span<std::byte> JobKmsSimpleSurface::data() noexcept
{
    if (!m_data || m_size == 0)
        return {};

    return {m_data, m_size};
}

std::span<const std::byte> JobKmsSimpleSurface::data() const noexcept
{
    if (!m_data || m_size == 0)
        return {};

    return {m_data, m_size};
}

bool JobKmsSimpleSurface::valid() const noexcept
{
    return m_device &&
           m_device->isOpen() &&
           m_handle         != 0 &&
           m_framebufferId  != 0 &&
           m_data           != nullptr &&
           m_width          != 0 &&
           m_height         != 0 &&
           m_stride         != 0 &&
           m_size           != 0 &&
           dumbValidFormats(m_format);
}

std::uint32_t JobKmsSimpleSurface::handle() const noexcept
{
    return m_handle;
}

std::uint32_t JobKmsSimpleSurface::framebufferId() const noexcept
{
    return m_framebufferId;
}

bool JobKmsSimpleSurface::createDumbBuffer()
{
    if (!m_device->dumbBuffers()) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] DRM device does not support dumb buffers.");
        return false;
    }
    const std::uint8_t colorMode = dumbColorMode(m_format);
    if (colorMode == 0) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Render format has no dumb-buffer color mode.");
        return false;
    }

    drm_mode_create_dumb db{};
    db.width    = m_width;
    db.height   = m_height;
    db.bpp      = colorMode;
    db.flags    = 0;

    if (::ioctl(m_device->fd(), DRM_IOCTL_MODE_CREATE_DUMB, &db) < 0) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Failed to create dumb buffer: {}", std::strerror(errno) );
        return false;
    }

    m_handle = db.handle;
    m_stride = db.pitch;
    m_size   = db.size;

    return true;
}

bool JobKmsSimpleSurface::createFramebuffer()
{
    const std::uint32_t format = dumbFramebufferFormat(m_format);
    if (format == DRM_FORMAT_INVALID)
        return false;

    std::uint32_t handles[4]{};
    std::uint32_t pitches[4]{};
    std::uint32_t offsets[4]{};

    handles[0] = m_handle;
    pitches[0] = static_cast<std::uint32_t>(m_stride);

    if (drmModeAddFB2(m_device->fd(), m_width, m_height, format, handles, pitches, offsets, &m_framebufferId, 0) != 0) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] drmModeAddFB2 failed: {}", std::strerror(errno));
        return false;
    }

    return true;
}

bool JobKmsSimpleSurface::map()
{
    drm_mode_map_dumb map{};
    map.handle = m_handle;
    if (::ioctl(m_device->fd(), DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Failed to prepare dumb buffer mapping: {}", std::strerror(errno));
        return false;
    }

    void *data = ::mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_device->fd(), map.offset);
    if (data == MAP_FAILED) {
        JOB_LOG_ERROR("[JobKmsSimpleSurface] Failed to mmap dumb buffer: {}", std::strerror(errno));
        return false;
    }

    m_data = static_cast<std::byte *>(data);
    return true;
}

void JobKmsSimpleSurface::unmap() noexcept
{
    if (!m_data){
#ifndef NDEBUG
        JOB_LOG_WARN("[JobKmsSimpleSurface] unmap Data is a nullptr");
#endif
        return;
    }

    if (::munmap(m_data, m_size) != 0)
        JOB_LOG_WARN("[JobKmsSimpleSurface] Failed to unmap dumb buffer: {}", std::strerror(errno));


    m_data = nullptr;
}

} // namespace job::render