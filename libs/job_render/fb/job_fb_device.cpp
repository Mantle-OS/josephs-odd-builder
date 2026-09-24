#include "job_fb_device.h"

#include <cerrno>
#include <fcntl.h>
#include <utility>

#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/fb.h>

#include <job_logger.h>

#include "job_fb_format.h"

namespace job::render {

JobFbDevice::JobFbDevice(std::filesystem::path path) :
    m_path(std::move(path))
{
}

JobFbDevice::~JobFbDevice()
{
    closeDevice();
}

bool JobFbDevice::openDevice()
{
    if (isOpen())
        return true;

    m_fd = ::open(m_path.c_str(), O_RDWR | O_CLOEXEC);
    if (m_fd < 0) {
        JOB_LOG_ERROR("[JobFbDevice] Failed to open framebuffer {}.", m_path);
        return false;
    }

    if (!refresh()) {
        JOB_LOG_ERROR("[JobFbDevice] Failed to query framebuffer {}.", m_path);
        closeDevice();
        return false;
    }

    if (!map()) {
        JOB_LOG_ERROR("[JobFbDevice] Failed to map framebuffer {}.", m_path);
        closeDevice();
        return false;
    }

    return true;
}

void JobFbDevice::closeDevice()
{
    unmap();

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_fixed = {};
    m_variable = {};
    m_format = {};
}

ssize_t JobFbDevice::read(char *buffer, size_t maxlen)
{
    if (!isOpen() || !buffer || maxlen == 0)
        return -1;

    const ssize_t bytesRead = ::read(m_fd, buffer, maxlen);
    if (bytesRead > 0 && m_readCallback)
        m_readCallback(buffer, static_cast<size_t>(bytesRead));

    return bytesRead;
}

ssize_t JobFbDevice::write(const char *data, size_t len)
{
    if (!isOpen() || !data || len == 0)
        return -1;

    return ::write(m_fd, data, len);
}

bool JobFbDevice::isOpen() const
{
    return m_fd >= 0;
}

int JobFbDevice::fd() const
{
    return m_fd;
}

void JobFbDevice::setNonBlocking(bool enabled)
{
    if (!isOpen())
        return;

    const int flags = ::fcntl(m_fd, F_GETFL);
    if (flags < 0)
        return;

    const int newFlags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (newFlags == flags)
        return;

    if (::fcntl(m_fd, F_SETFL, newFlags) < 0)
        JOB_LOG_ERROR("[JobFbDevice] Failed to change non-blocking state for {}.", m_path);
}

void JobFbDevice::setReadCallback(ReadCallback cb)
{
    m_readCallback = std::move(cb);
}

bool JobFbDevice::open(std::filesystem::path path)
{
    closeDevice();
    m_path = std::move(path);
    return openDevice();
}

bool JobFbDevice::refresh()
{
    if (!isOpen())
        return false;

    if (!queryFixedInfo())
        return false;

    if (!queryVariableInfo())
        return false;

    if (!detectFormat())
        return false;

    return true;
}

bool JobFbDevice::map()
{
    if (!isOpen() || mappedSize() == 0)
        return false;

    unmap();
    m_map = io::JobMmap::createUniq(m_fd, mappedSize(), 0, io::JobFile::Access::ReadWrite, false);
    if (!m_map || !m_map->isValid()) {
        JOB_LOG_ERROR("[JobFbDevice] Failed to map framebuffer {}.", m_path);
        m_map.reset();
        return false;
    }

    return true;
}

void JobFbDevice::unmap() noexcept
{
    m_map.reset();
}

std::byte *JobFbDevice::data() noexcept
{
    if (!mapped())
        return nullptr;

    return static_cast<std::byte *>(m_map->addr());
}

const std::byte *JobFbDevice::data() const noexcept
{
    if (!mapped())
        return nullptr;

    return static_cast<const std::byte *>(m_map->addr());
}

bool JobFbDevice::queryFixedInfo()
{
    if (!isOpen())
        return false;

    fb_fix_screeninfo info{};
    if (::ioctl(m_fd, FBIOGET_FSCREENINFO, &info) < 0) {
        JOB_LOG_ERROR("[JobFbDevice] FBIOGET_FSCREENINFO failed for {}.", m_path);
        return false;
    }

    m_fixed = JobFbFixedInfo::fromFb(info);
    return m_fixed.valid();
}

bool JobFbDevice::queryVariableInfo()
{
    if (!isOpen())
        return false;

    fb_var_screeninfo info{};
    if (::ioctl(m_fd, FBIOGET_VSCREENINFO, &info) < 0) {
        JOB_LOG_ERROR("[JobFbDevice] FBIOGET_VSCREENINFO failed for {}.", m_path);
        return false;
    }

    m_variable = JobFbVariableInfo::fromFb(info);
    return m_variable.valid();
}

bool JobFbDevice::detectFormat()
{
    m_format = JobFbFormat::detect(m_fixed, m_variable);
    if (!m_format.valid()) {
        JOB_LOG_ERROR("[JobFbDevice] Unsupported framebuffer format for {}.", m_path);
        return false;
    }

    return true;
}

}