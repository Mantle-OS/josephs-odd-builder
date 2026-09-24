#include "job_input_device.h"

#include <utility>

namespace job::input {

JobInputDevice::JobInputDevice() = default;
JobInputDevice::~JobInputDevice() = default;

JobInputDevice::Ptr JobInputDevice::createShared()
{
    return std::make_shared<JobInputDevice>();
}

JobInputDevice::UPtr JobInputDevice::createUniq()
{
    return std::make_unique<JobInputDevice>();
}

InputDeviceId JobInputDevice::id() const noexcept
{
    return m_id;
}

void JobInputDevice::setId(InputDeviceId id) noexcept
{
    m_id = id;
}

const std::string &JobInputDevice::name() const noexcept
{
    return m_name;
}

void JobInputDevice::setName(std::string name)
{
    m_name = std::move(name);
}

const std::string &JobInputDevice::sysName() const noexcept
{
    return m_sysName;
}

void JobInputDevice::setSysName(std::string sysName)
{
    m_sysName = std::move(sysName);
}

std::uint32_t JobInputDevice::vendorId() const noexcept
{
    return m_vendorId;
}

void JobInputDevice::setVendorId(std::uint32_t vendorId) noexcept
{
    m_vendorId = vendorId;
}

std::uint32_t JobInputDevice::productId() const noexcept
{
    return m_productId;
}

void JobInputDevice::setProductId(std::uint32_t productId) noexcept
{
    m_productId = productId;
}

InputCapability JobInputDevice::capabilities() const noexcept
{
    return m_capabilities;
}

void JobInputDevice::setCapabilities(InputCapability capabilities) noexcept
{
    m_capabilities = capabilities;
}

bool JobInputDevice::hasCapability(InputCapability capability) const noexcept
{
    return job::input::hasCapability(m_capabilities, capability);
}

} // namespace job::input