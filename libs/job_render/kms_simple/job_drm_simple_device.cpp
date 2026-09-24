#include "job_drm_simple_device.h"

#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <utility>
#include <unistd.h>

#include <xf86drm.h>

#include <job_logger.h>

namespace job::render {

JobDrmSimpleDevice::JobDrmSimpleDevice() = default;

JobDrmSimpleDevice::JobDrmSimpleDevice(std::string path) :
    m_path(std::move(path))
{
}

JobDrmSimpleDevice::~JobDrmSimpleDevice()
{
    closeDevice();
}

JobDrmSimpleDevice::Ptr JobDrmSimpleDevice::createShared()
{
    return std::make_shared<JobDrmSimpleDevice>();
}

JobDrmSimpleDevice::Ptr JobDrmSimpleDevice::createShared(std::string path)
{
    return std::make_shared<JobDrmSimpleDevice>(std::move(path));
}

JobDrmSimpleDevice::UPtr JobDrmSimpleDevice::createUniq()
{
    return std::make_unique<JobDrmSimpleDevice>();
}

JobDrmSimpleDevice::UPtr JobDrmSimpleDevice::createUniq(std::string path)
{
    return std::make_unique<JobDrmSimpleDevice>(std::move(path));
}

bool JobDrmSimpleDevice::openDevice()
{
    if (isOpen())
        return true;

    m_fd = ::open(m_path.c_str(), O_RDWR | O_CLOEXEC);

    if (m_fd < 0) {
        JOB_LOG_ERROR("[JobDrmSimpleDevice] Failed to open {}: {}", m_path, std::strerror(errno));
        return false;
    }

    readCapabilities();

    if (!refreshResources()) {
        JOB_LOG_ERROR("[JobDrmSimpleDevice] Failed to read DRM resources from {}.", m_path);
        closeDevice();
        return false;
    }

    return true;
}

void JobDrmSimpleDevice::closeDevice()
{
    clearResources();

    m_dumbBuffers = false;
    m_atomic = false;

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_readCallback = nullptr;
}

ssize_t JobDrmSimpleDevice::read(char *buffer, std::size_t maxlen)
{
    if (!isOpen() || !buffer || maxlen == 0)
        return -1;

    return ::read(m_fd, buffer, maxlen);
}

ssize_t JobDrmSimpleDevice::write(const char *data, std::size_t len)
{
    if (!isOpen() || !data || len == 0)
        return -1;

    return ::write(m_fd, data, len);
}

bool JobDrmSimpleDevice::isOpen() const
{
    return m_fd >= 0;
}

int JobDrmSimpleDevice::fd() const
{
    return m_fd;
}

void JobDrmSimpleDevice::setNonBlocking(bool enabled)
{
    if (!isOpen())
        return;

    const int flags = ::fcntl(m_fd, F_GETFL, 0);

    if (flags < 0) {
        JOB_LOG_WARN("[JobDrmSimpleDevice] Failed to read flags for {}: {}", m_path, std::strerror(errno));
        return;
    }

    int updated = flags;

    if (enabled)
        updated |= O_NONBLOCK;
    else
        updated &= ~O_NONBLOCK;

    if (::fcntl(m_fd, F_SETFL, updated) < 0)
        JOB_LOG_WARN("[JobDrmSimpleDevice] Failed to update flags for {}: {}", m_path, std::strerror(errno));
}

void JobDrmSimpleDevice::setReadCallback(ReadCallback callback)
{
    m_readCallback = std::move(callback);
}

const std::string &JobDrmSimpleDevice::path() const noexcept
{
    return m_path;
}

void JobDrmSimpleDevice::setPath(std::string path)
{
    if (isOpen()) {
        JOB_LOG_WARN("[JobDrmSimpleDevice] Cannot change path while device is open.");
        return;
    }

    m_path = std::move(path);
}

bool JobDrmSimpleDevice::refreshResources()
{
    if (!isOpen())
        return false;

    DrmModeResPtr resources{drmModeGetResources(m_fd)};

    if (!resources) {
        JOB_LOG_ERROR("[JobDrmSimpleDevice] drmModeGetResources failed for {}.", m_path);
        return false;
    }

    m_resources = std::move(resources);
    return true;
}

drmModeRes *JobDrmSimpleDevice::resources() noexcept
{
    return m_resources.get();
}

const drmModeRes *JobDrmSimpleDevice::resources() const noexcept
{
    return m_resources.get();
}

std::size_t JobDrmSimpleDevice::fbCount() const noexcept
{
    return m_resources ? static_cast<std::size_t>(m_resources->count_fbs) : 0;
}

std::span<const std::uint32_t> JobDrmSimpleDevice::fbs() const noexcept
{
    if (!m_resources || !m_resources->fbs || m_resources->count_fbs <= 0)
        return {};

    return {
        m_resources->fbs,
        static_cast<std::size_t>(m_resources->count_fbs)
    };
}

std::size_t JobDrmSimpleDevice::crtcCount() const noexcept
{
    return m_resources ? static_cast<std::size_t>(m_resources->count_crtcs) : 0;
}

std::span<const std::uint32_t> JobDrmSimpleDevice::crtcs() const noexcept
{
    if (!m_resources || !m_resources->crtcs || m_resources->count_crtcs <= 0)
        return {};

    return {
        m_resources->crtcs,
        static_cast<std::size_t>(m_resources->count_crtcs)
    };
}

std::size_t JobDrmSimpleDevice::connectorCount() const noexcept
{
    return m_resources ? static_cast<std::size_t>(m_resources->count_connectors) : 0;
}

std::span<const std::uint32_t> JobDrmSimpleDevice::connectors() const noexcept
{
    if (!m_resources || !m_resources->connectors || m_resources->count_connectors <= 0)
        return {};

    return {
        m_resources->connectors,
        static_cast<std::size_t>(m_resources->count_connectors)
    };
}

std::size_t JobDrmSimpleDevice::encoderCount() const noexcept
{
    return m_resources ? static_cast<std::size_t>(m_resources->count_encoders) : 0;
}

std::span<const std::uint32_t> JobDrmSimpleDevice::encoders() const noexcept
{
    if (!m_resources || !m_resources->encoders || m_resources->count_encoders <= 0)
        return {};

    return {
        m_resources->encoders,
        static_cast<std::size_t>(m_resources->count_encoders)
    };
}

std::uint32_t JobDrmSimpleDevice::minWidth() const noexcept
{
    return m_resources ? m_resources->min_width : 0;
}

std::uint32_t JobDrmSimpleDevice::maxWidth() const noexcept
{
    return m_resources ? m_resources->max_width : 0;
}

std::uint32_t JobDrmSimpleDevice::minHeight() const noexcept
{
    return m_resources ? m_resources->min_height : 0;
}

std::uint32_t JobDrmSimpleDevice::maxHeight() const noexcept
{
    return m_resources ? m_resources->max_height : 0;
}

bool JobDrmSimpleDevice::dumbBuffers() const noexcept
{
    return m_dumbBuffers;
}

bool JobDrmSimpleDevice::atomic() const noexcept
{
    return m_atomic;
}

void JobDrmSimpleDevice::DrmModeResDeleter::operator()(drmModeRes *resources) const noexcept
{
    if (resources)
        drmModeFreeResources(resources);
}

void JobDrmSimpleDevice::clearResources() noexcept
{
    m_resources.reset();
}

void JobDrmSimpleDevice::readCapabilities()
{
    m_dumbBuffers = false;
    m_atomic = false;

    if (!isOpen())
        return;

    std::uint64_t value = 0;

    if (drmGetCap(m_fd, DRM_CAP_DUMB_BUFFER, &value) == 0)
        m_dumbBuffers = value != 0;

    if (drmSetClientCap(m_fd, DRM_CLIENT_CAP_ATOMIC, 1) == 0)
        m_atomic = true;
}

} // namespace job::render