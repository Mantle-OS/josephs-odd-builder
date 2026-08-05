#include "job_ggml_device_caps.h"

namespace job::ggml {

JobGgmlDeviceCaps::JobGgmlDeviceCaps(ggml_backend_dev_caps caps)
{
    setCaps(caps);
}

bool JobGgmlDeviceCaps::operator==(const JobGgmlDeviceCaps &other) const noexcept
{
    return m_async              == other.m_async &&
           m_hostBuffer         == other.m_hostBuffer &&
           m_bufferFromHostPtr  == other.m_bufferFromHostPtr &&
           m_events             == other.m_events;
}

bool JobGgmlDeviceCaps::operator!=(const JobGgmlDeviceCaps &other) const noexcept
{
    return !(*this == other);
}

bool JobGgmlDeviceCaps::async() const noexcept
{
    return m_async;
}

void JobGgmlDeviceCaps::setAsync(bool async) noexcept
{
    if (m_async != async)
        m_async = async;
}

bool JobGgmlDeviceCaps::hostBuffer() const noexcept
{
    return m_hostBuffer;
}

void JobGgmlDeviceCaps::setHostBuffer(bool hostBuffer) noexcept
{
    if (m_hostBuffer != hostBuffer)
        m_hostBuffer = hostBuffer;
}

bool JobGgmlDeviceCaps::bufferFromHostPtr() const noexcept
{
    return m_bufferFromHostPtr;
}

void JobGgmlDeviceCaps::setBufferFromHostPtr(bool bufferFromHostPtr) noexcept
{
    if (m_bufferFromHostPtr != bufferFromHostPtr)
        m_bufferFromHostPtr = bufferFromHostPtr;
}

bool JobGgmlDeviceCaps::events() const noexcept
{
    return m_events;
}

void JobGgmlDeviceCaps::setEvents(bool events) noexcept
{
    if (m_events != events)
        m_events = events;
}

void JobGgmlDeviceCaps::setCaps(ggml_backend_dev_caps other) noexcept
{
    setAsync(other.async);
    setHostBuffer(other.host_buffer);
    setBufferFromHostPtr(other.buffer_from_host_ptr);
    setEvents(other.events);

    m_caps = other;
}

ggml_backend_dev_caps JobGgmlDeviceCaps::caps() noexcept
{
    ggml_backend_dev_caps ret{defaultCaps()};

    ret.async                = async();
    ret.host_buffer          = hostBuffer();
    ret.buffer_from_host_ptr = bufferFromHostPtr();
    ret.events               = events();

    m_caps = ret;
    return m_caps;
}

void JobGgmlDeviceCaps::resetCaps() noexcept
{
    m_caps = defaultCaps();
    setCaps(m_caps);
}

} // namespace job::ggml