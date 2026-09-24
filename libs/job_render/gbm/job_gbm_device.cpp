#include "job_gbm_device.h"

namespace job::render {

void JobGbmDevice::GbmDeviceDeleter::operator()(gbm_device *device) const noexcept
{
    if (device)
        gbm_device_destroy(device);
}

bool JobGbmDevice::create(int drmFd)
{
    reset();
    if (drmFd < 0)
        return false;

    m_device.reset(gbm_create_device(drmFd));
    if (!m_device)
        return false;

    m_drmFd = drmFd;

    return true;
}

bool JobGbmDevice::supportsFormat(JobGbmFormat format, std::uint32_t usage) const noexcept
{
    if (!m_device || !format.valid())
        return false;
    return gbm_device_is_format_supported(m_device.get(), format.value(), usage) != 0;
}

}