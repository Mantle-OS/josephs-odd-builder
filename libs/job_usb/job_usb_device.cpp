#include "job_usb_device.h"

#include <utility>

namespace job::usb {

JobUsbDevice::Ptr JobUsbDevice::createShared()
{
    return std::make_shared<JobUsbDevice>();
}

JobUsbDevice::Ptr JobUsbDevice::createShared(uint64_t uid,
                                             JobUsbDescriptor descriptor,
                                             JobUsbLocation location,
                                             JobUsbStrings strings,
                                             JobUsbState state)
{
    return std::make_shared<JobUsbDevice>(uid,
                                          std::move(descriptor),
                                          std::move(location),
                                          std::move(strings),
                                          std::move(state));
}

JobUsbDevice::UPtr JobUsbDevice::createUniq()
{
    return std::make_unique<JobUsbDevice>();
}

JobUsbDevice::UPtr JobUsbDevice::createUniq(uint64_t uid,
                                            JobUsbDescriptor descriptor,
                                            JobUsbLocation location,
                                            JobUsbStrings strings,
                                            JobUsbState state)
{
    return std::make_unique<JobUsbDevice>(uid,
                                          std::move(descriptor),
                                          std::move(location),
                                          std::move(strings),
                                          std::move(state));
}

} // namespace job::usb