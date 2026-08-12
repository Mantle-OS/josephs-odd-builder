#include "job_ggml_device_props.h"

#include <utility>

namespace job::ggml {

JobGgmlDeviceProps::JobGgmlDeviceProps(ggml_backend_dev_props deviceProps)
{
    setProps(deviceProps);
}

bool JobGgmlDeviceProps::operator==(const JobGgmlDeviceProps &other) const noexcept
{
    const bool capsEqual = m_caps && other.m_caps ? *m_caps == *other.m_caps : !m_caps && !other.m_caps;
    return m_ggmlDeviceType == other.m_ggmlDeviceType &&
           m_name           == other.m_name           &&
           m_description    == other.m_description    &&
           m_memoryFree     == other.m_memoryFree     &&
           m_memoryTotal    == other.m_memoryTotal    &&
           m_deviceId       == other.m_deviceId       &&
           capsEqual;
}

bool JobGgmlDeviceProps::operator!=(const JobGgmlDeviceProps &other) const noexcept
{
    return !(*this == other);
}

enum ggml_backend_dev_type JobGgmlDeviceProps::type() const noexcept
{
    return m_ggmlDeviceType;
}

void JobGgmlDeviceProps::setGgmlType(enum ggml_backend_dev_type type)
{
    if (m_ggmlDeviceType != type)
        m_ggmlDeviceType = type;
}

JobGgmlDeviceType JobGgmlDeviceProps::deviceType() const noexcept
{
    return  fromGgmlBackendDeviceType(m_ggmlDeviceType);
}

void JobGgmlDeviceProps::setDeviceType(JobGgmlDeviceType type)
{
    auto dType = toGgmlBackendDeviceType(type);
    if (m_ggmlDeviceType != dType)
        m_ggmlDeviceType = dType;
}

const std::string &JobGgmlDeviceProps::name() const noexcept
{
    return m_name;
}

void JobGgmlDeviceProps::setName(const std::string &name)
{
    if (m_name != name && !name.empty())
        m_name = name;
}

const std::string &JobGgmlDeviceProps::description() const noexcept
{
    return m_description;
}

void JobGgmlDeviceProps::setDescription(const std::string &description)
{
    if (m_description != description && !description.empty())
        m_description = description;
}

const std::size_t &JobGgmlDeviceProps::memoryFree() const noexcept
{
    return m_memoryFree;
}

void JobGgmlDeviceProps::setMemoryFree(const std::size_t &memoryFree)
{
    if (m_memoryFree != memoryFree)
        m_memoryFree = memoryFree;
}

const std::size_t &JobGgmlDeviceProps::memoryTotal() const noexcept
{
    return m_memoryTotal;
}

void JobGgmlDeviceProps::setMemoryTotal(const std::size_t &memoryTotal)
{
    if (m_memoryTotal != memoryTotal)
        m_memoryTotal = memoryTotal;
}

const std::string &JobGgmlDeviceProps::deviceId() const noexcept
{
    return m_deviceId;
}

void JobGgmlDeviceProps::setDeviceId(const std::string &deviceId)
{
    if (m_deviceId != deviceId && !deviceId.empty())
        m_deviceId = deviceId;
}

JobGgmlDeviceCaps *JobGgmlDeviceProps::caps() noexcept
{
    return m_caps.get();
}

const JobGgmlDeviceCaps *JobGgmlDeviceProps::caps() const noexcept
{
    return m_caps.get();
}

void JobGgmlDeviceProps::setCaps(JobGgmlDeviceCaps::UPtr caps)
{
    if (caps)
        m_caps = std::move(caps);
}

ggml_backend_dev_props JobGgmlDeviceProps::props()
{
    ggml_backend_dev_props ret{defaultProps()};

    ret.name         = m_name.c_str();
    ret.description  = m_description == "unknown" ? nullptr : m_description.c_str();
    ret.memory_free  = m_memoryFree;
    ret.memory_total = m_memoryTotal;
    ret.type         = m_ggmlDeviceType;
    ret.device_id    = m_deviceId == "unknown" ? nullptr : m_deviceId.c_str();

    if (m_caps)
        ret.caps = m_caps->caps();

    m_props = ret;

    return m_props;
}

void JobGgmlDeviceProps::setProps(ggml_backend_dev_props other)
{
    setName(other.name ? other.name : "cpu");
    setDescription(other.description ? other.description : "unknown");
    setMemoryFree(other.memory_free);
    setMemoryTotal(other.memory_total);
    setGgmlType(other.type);
    setDeviceId(other.device_id ? other.device_id : "unknown");

    if (!m_caps)
        m_caps = JobGgmlDeviceCaps::createUniq(other.caps);
    else
        m_caps->setCaps(other.caps);

    m_props = other;
}

void JobGgmlDeviceProps::resetProps()
{
    m_props = defaultProps();
    setProps(m_props);
}

} // namespace job::ggml